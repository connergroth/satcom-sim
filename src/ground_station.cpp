#include "ground_station.hpp"
#include <iostream>
#include <iomanip>

GroundStation::GroundStation(Link& link, const Config& config)
    : link_(link), config_(config), rng_(config.seed + 1000) {
    // Open log file
    log_file_.open(config_.log_file, std::ios::out | std::ios::trunc);
    if (log_file_.is_open()) {
        log_file_ << Telemetry::csv_header() << std::endl;
    }
}

GroundStation::~GroundStation() {
    stop();
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

void GroundStation::start() {
    if (running_.exchange(true)) {
        return;  // Already running
    }
    start_time_ = std::chrono::steady_clock::now();
    last_command_time_ = start_time_;
    thread_ = std::thread(&GroundStation::run, this);
}

void GroundStation::stop() {
    if (running_.exchange(false)) {
        if (thread_.joinable()) {
            thread_.join();
        }
    }
}

void GroundStation::run() {
    while (running_) {
        receive_telemetry();
        send_periodic_commands();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void GroundStation::receive_telemetry() {
    Packet pkt;
    while (link_.recv_sat_to_gs(pkt, std::chrono::milliseconds(0))) {
        if (!pkt.verify_crc()) {
            if (config_.verbose) {
                std::cout << "[GS ] NAK seq=" << pkt.seq << " (bad CRC)" << std::endl;
            }
            naks_sent_++;
            // Send NAK
            Packet nak;
            nak.type = PacketType::NakPkt;
            nak.seq = pkt.seq;
            nak.payload = "";
            nak.payload_size = 0;
            nak.compute_crc();
            link_.send_gs_to_sat(nak);
            continue;
        }

        if (pkt.type == PacketType::TelemetryPkt) {
            // Check for duplicate
            if (pkt.seq < rx_seq_expected_) {
                // Duplicate, but still ACK
                if (config_.verbose) {
                    std::cout << "[GS ] RX Telemetry seq=" << pkt.seq << " (duplicate) → ACK" << std::endl;
                }
                Packet ack;
                ack.type = PacketType::AckPkt;
                ack.seq = pkt.seq;
                ack.payload = "";
                ack.payload_size = 0;
                ack.compute_crc();
                link_.send_gs_to_sat(ack);
                continue;
            }

            rx_seq_expected_ = pkt.seq + 1;

            try {
                Telemetry telem = Telemetry::from_json(pkt.payload);
                telemetry_received_++;

                if (config_.verbose) {
                    std::cout << std::fixed << std::setprecision(1);
                    std::cout << "[GS ] RX Telemetry seq=" << pkt.seq
                              << " temp=" << telem.temperature_c << "C"
                              << " batt=" << telem.battery_pct << "%"
                              << " alt=" << telem.orbit_altitude_km << "km"
                              << " → ACK" << std::endl;
                }

                // Log telemetry
                log_telemetry(telem);

                // Send ACK
                Packet ack;
                ack.type = PacketType::AckPkt;
                ack.seq = pkt.seq;
                ack.payload = "";
                ack.payload_size = 0;
                ack.compute_crc();
                link_.send_gs_to_sat(ack);

            } catch (const std::exception& e) {
                if (config_.verbose) {
                    std::cout << "[GS ] ERROR: failed to parse telemetry: " << e.what() << std::endl;
                }
                // Send NAK
                Packet nak;
                nak.type = PacketType::NakPkt;
                nak.seq = pkt.seq;
                nak.payload = "";
                nak.payload_size = 0;
                nak.compute_crc();
                link_.send_gs_to_sat(nak);
                naks_sent_++;
            }
        }
    }
}

void GroundStation::send_periodic_commands() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();
    auto since_last_cmd = std::chrono::duration_cast<std::chrono::seconds>(now - last_command_time_).count();

    // Send commands at intervals
    if (since_last_cmd >= 4) {
        if (elapsed < 8) {
            // First 8 seconds: adjust orientation
            Command cmd;
            cmd.type = CommandType::AdjustOrientation;
            std::uniform_real_distribution<double> angle_dist(-2.0, 2.0);
            cmd.d_pitch = angle_dist(rng_);
            cmd.d_yaw = angle_dist(rng_);
            cmd.d_roll = angle_dist(rng_);
            send_command_with_retry(cmd);
        } else if (elapsed >= 8 && elapsed < 12) {
            // Mid-run: thrust burn
            Command cmd;
            cmd.type = CommandType::ThrustBurn;
            cmd.burn_seconds = 2.0;
            send_command_with_retry(cmd);
        }
        last_command_time_ = now;
    }
}

void GroundStation::send_command_with_retry(const Command& cmd) {
    Packet pkt;
    pkt.type = PacketType::CommandPkt;
    pkt.seq = tx_seq_++;
    pkt.payload = cmd.serialize();
    pkt.payload_size = static_cast<uint32_t>(pkt.payload.size());
    pkt.compute_crc();

    if (config_.verbose) {
        std::cout << "[GS ] CMD TX " << cmd.name() << " seq=" << pkt.seq;
        if (cmd.type == CommandType::AdjustOrientation) {
            std::cout << " d=(" << std::fixed << std::setprecision(1)
                      << cmd.d_pitch << "," << cmd.d_yaw << "," << cmd.d_roll << ")";
        } else if (cmd.type == CommandType::ThrustBurn) {
            std::cout << " t=" << cmd.burn_seconds << "s";
        }
        std::cout << std::endl;
    }

    // Send with retry logic
    bool success = false;
    for (int retry = 0; retry <= config_.max_retries && running_; ++retry) {
        if (retry > 0) {
            retries_++;
            if (config_.verbose) {
                std::cout << "[GS ] WARN: missed ACK for cmd seq=" << pkt.seq
                          << " → retry " << retry << "/" << config_.max_retries << std::endl;
            }
        }

        link_.send_gs_to_sat(pkt);

        // Wait for ACK
        if (wait_for_ack(pkt.seq, std::chrono::milliseconds(config_.ack_timeout_ms))) {
            success = true;
            break;
        }
    }

    if (success) {
        commands_sent_++;
    } else if (config_.verbose && running_) {
        std::cout << "[GS ] ERROR: failed to send command seq=" << pkt.seq
                  << " after " << config_.max_retries << " retries" << std::endl;
    }
}

bool GroundStation::wait_for_ack(uint32_t seq, std::chrono::milliseconds timeout) {
    Packet pkt;
    if (link_.recv_sat_to_gs(pkt, timeout)) {
        if (pkt.type == PacketType::AckPkt && pkt.seq == seq) {
            if (config_.verbose) {
                std::cout << "[GS ] RX ACK seq=" << seq << std::endl;
            }
            return true;
        } else if (pkt.type == PacketType::NakPkt && pkt.seq == seq) {
            if (config_.verbose) {
                std::cout << "[GS ] RX NAK seq=" << seq << std::endl;
            }
            return false;
        }
    }
    return false;
}

void GroundStation::log_telemetry(const Telemetry& t) {
    if (log_file_.is_open()) {
        log_file_ << t.to_csv() << std::endl;
    }
}
