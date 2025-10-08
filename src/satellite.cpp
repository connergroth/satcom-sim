#include "satellite.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>

Satellite::Satellite(Link& link, const Config& config)
    : link_(link), config_(config), rng_(config.seed) {}

Satellite::~Satellite() {
    stop();
}

void Satellite::start() {
    if (running_.exchange(true)) {
        return;  // Already running
    }
    thread_ = std::thread(&Satellite::run, this);
}

void Satellite::stop() {
    if (running_.exchange(false)) {
        if (thread_.joinable()) {
            thread_.join();
        }
    }
}

void Satellite::run() {
    auto last_telemetry = std::chrono::steady_clock::now();
    auto last_update = std::chrono::steady_clock::now();

    const auto telemetry_period = std::chrono::milliseconds(
        static_cast<long long>(1000.0 / config_.telemetry_rate_hz)
    );

    while (running_) {
        auto now = std::chrono::steady_clock::now();

        // Update state
        auto dt = std::chrono::duration<double>(now - last_update).count();
        update_state(dt);
        last_update = now;

        // Check for anomalies
        check_anomalies();

        // Send telemetry at specified rate
        if (now - last_telemetry >= telemetry_period) {
            send_telemetry();
            last_telemetry = now;
        }

        // Process incoming commands
        process_commands();

        // Sleep briefly to avoid busy-wait
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void Satellite::send_telemetry() {
    Telemetry telem;
    telem.ts = std::chrono::steady_clock::now();
    telem.temperature_c = temperature_c_;
    telem.battery_pct = battery_pct_;
    telem.orbit_altitude_km = orbit_altitude_km_;
    telem.pitch_deg = pitch_deg_;
    telem.yaw_deg = yaw_deg_;
    telem.roll_deg = roll_deg_;

    // Create packet
    Packet pkt;
    pkt.type = PacketType::TelemetryPkt;
    pkt.seq = tx_seq_++;
    pkt.payload = telem.to_json();
    pkt.payload_size = static_cast<uint32_t>(pkt.payload.size());
    pkt.compute_crc();

    if (config_.verbose) {
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "[SAT] TX Telemetry seq=" << pkt.seq
                  << " crc=0x" << std::hex << pkt.crc16 << std::dec
                  << " temp=" << temperature_c_ << "C"
                  << " batt=" << battery_pct_ << "%"
                  << " alt=" << orbit_altitude_km_ << "km"
                  << " euler=(" << pitch_deg_ << "," << yaw_deg_ << "," << roll_deg_ << ")"
                  << (safe_mode_ ? " [SAFE MODE]" : "")
                  << std::endl;
    }

    // Send with retry logic
    bool success = false;
    for (int retry = 0; retry <= config_.max_retries && running_; ++retry) {
        if (retry > 0) {
            retries_++;
            if (config_.verbose) {
                std::cout << "[SAT] WARN: missed ACK for seq=" << pkt.seq
                          << " → retry " << retry << "/" << config_.max_retries << std::endl;
            }
        }

        link_.send_sat_to_gs(pkt);

        // Wait for ACK
        if (wait_for_ack(pkt.seq, std::chrono::milliseconds(config_.ack_timeout_ms))) {
            success = true;
            break;
        }
    }

    if (success) {
        telemetry_sent_++;
    } else if (config_.verbose && running_) {
        std::cout << "[SAT] ERROR: failed to send telemetry seq=" << pkt.seq
                  << " after " << config_.max_retries << " retries" << std::endl;
    }
}

void Satellite::process_commands() {
    Packet pkt;
    while (link_.recv_gs_to_sat(pkt, std::chrono::milliseconds(0))) {
        if (!pkt.verify_crc()) {
            if (config_.verbose) {
                std::cout << "[SAT] NAK seq=" << pkt.seq << " (bad CRC)" << std::endl;
            }
            // Send NAK
            Packet nak;
            nak.type = PacketType::NakPkt;
            nak.seq = pkt.seq;
            nak.payload = "";
            nak.payload_size = 0;
            nak.compute_crc();
            link_.send_sat_to_gs(nak);
            continue;
        }

        if (pkt.type == PacketType::CommandPkt) {
            // Check for duplicate
            if (pkt.seq < rx_seq_expected_) {
                // Duplicate, but still ACK
                Packet ack;
                ack.type = PacketType::AckPkt;
                ack.seq = pkt.seq;
                ack.payload = "";
                ack.payload_size = 0;
                ack.compute_crc();
                link_.send_sat_to_gs(ack);
                continue;
            }

            rx_seq_expected_ = pkt.seq + 1;

            try {
                Command cmd = Command::deserialize(pkt.payload);
                commands_received_++;

                if (config_.verbose) {
                    std::cout << "[SAT] CMD RX " << cmd.name() << " seq=" << pkt.seq;
                }

                // Execute command
                switch (cmd.type) {
                    case CommandType::AdjustOrientation:
                        pitch_deg_ += cmd.d_pitch;
                        yaw_deg_ += cmd.d_yaw;
                        roll_deg_ += cmd.d_roll;
                        if (config_.verbose) {
                            std::cout << " d=(" << cmd.d_pitch << "," << cmd.d_yaw
                                      << "," << cmd.d_roll << ") → applied" << std::endl;
                        }
                        break;

                    case CommandType::ThrustBurn:
                        if (safe_mode_) {
                            if (config_.verbose) {
                                std::cout << " → BLOCKED (safe mode)" << std::endl;
                            }
                        } else {
                            orbit_altitude_km_ += cmd.burn_seconds * 0.5;
                            battery_pct_ -= cmd.burn_seconds * 2.0;
                            if (config_.verbose) {
                                std::cout << " t=" << cmd.burn_seconds << "s → applied" << std::endl;
                            }
                        }
                        break;

                    case CommandType::EnterSafeMode:
                        safe_mode_ = true;
                        if (config_.verbose) {
                            std::cout << " → SAFE MODE ENABLED" << std::endl;
                        }
                        break;

                    case CommandType::Reboot:
                        if (config_.verbose) {
                            std::cout << " → rebooting..." << std::endl;
                        }
                        safe_mode_ = false;
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        if (config_.verbose) {
                            std::cout << "[SAT] Reboot complete" << std::endl;
                        }
                        break;
                }

                // Send ACK
                Packet ack;
                ack.type = PacketType::AckPkt;
                ack.seq = pkt.seq;
                ack.payload = "";
                ack.payload_size = 0;
                ack.compute_crc();
                link_.send_sat_to_gs(ack);

            } catch (const std::exception& e) {
                if (config_.verbose) {
                    std::cout << "[SAT] ERROR: failed to process command: " << e.what() << std::endl;
                }
                // Send NAK
                Packet nak;
                nak.type = PacketType::NakPkt;
                nak.seq = pkt.seq;
                nak.payload = "";
                nak.payload_size = 0;
                nak.compute_crc();
                link_.send_sat_to_gs(nak);
            }
        }
    }
}

void Satellite::update_state(double dt) {
    if (dt <= 0 || dt > 1.0) return;  // Sanity check

    // Simulate temperature variation
    std::uniform_real_distribution<double> temp_dist(-0.5, 0.5);
    temperature_c_ += temp_dist(rng_) * dt;

    // Battery drain (faster in safe mode due to heaters)
    battery_pct_ -= (safe_mode_ ? 0.2 : 0.1) * dt;
    battery_pct_ = std::max(0.0, battery_pct_);

    // Altitude decay due to drag
    orbit_altitude_km_ -= 0.001 * dt;

    // Attitude drift
    std::uniform_real_distribution<double> drift_dist(-0.05, 0.05);
    pitch_deg_ += drift_dist(rng_) * dt;
    yaw_deg_ += drift_dist(rng_) * dt;
    roll_deg_ += drift_dist(rng_) * dt;
}

void Satellite::check_anomalies() {
    if (!safe_mode_ && (temperature_c_ > 85.0 || battery_pct_ < 10.0)) {
        safe_mode_ = true;
        if (config_.verbose) {
            std::cout << "[SAT] ENTER SAFE MODE (";
            if (temperature_c_ > 85.0) std::cout << "high temp";
            if (temperature_c_ > 85.0 && battery_pct_ < 10.0) std::cout << ", ";
            if (battery_pct_ < 10.0) std::cout << "low battery";
            std::cout << ")" << std::endl;
        }
    }
}

bool Satellite::wait_for_ack(uint32_t seq, std::chrono::milliseconds timeout) {
    Packet pkt;
    if (link_.recv_gs_to_sat(pkt, timeout)) {
        if (pkt.type == PacketType::AckPkt && pkt.seq == seq) {
            return true;
        } else if (pkt.type == PacketType::NakPkt && pkt.seq == seq) {
            naks_received_++;
            return false;
        }
    }
    return false;
}
