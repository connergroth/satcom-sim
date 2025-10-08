#pragma once

#include "link.hpp"
#include "telemetry.hpp"
#include "commands.hpp"
#include "packet.hpp"
#include <atomic>
#include <thread>
#include <fstream>
#include <random>

/**
 * Ground station simulator running in its own thread.
 * Receives telemetry from satellite and sends commands.
 */
class GroundStation {
public:
    struct Config {
        int ack_timeout_ms = 150;
        int max_retries = 3;
        std::string log_file = "telemetry.log";
        bool verbose = false;
        unsigned int seed = 42;
    };

    GroundStation(Link& link, const Config& config);
    ~GroundStation();

    // Start/stop ground station thread
    void start();
    void stop();

    // Metrics
    uint64_t get_telemetry_received() const { return telemetry_received_; }
    uint64_t get_commands_sent() const { return commands_sent_; }
    uint64_t get_retries() const { return retries_; }
    uint64_t get_naks_sent() const { return naks_sent_; }

private:
    void run();
    void receive_telemetry();
    void send_periodic_commands();
    void send_command_with_retry(const Command& cmd);
    bool wait_for_ack(uint32_t seq, std::chrono::milliseconds timeout);
    void log_telemetry(const Telemetry& t);

    Link& link_;
    Config config_;
    std::atomic<bool> running_{false};
    std::thread thread_;
    std::ofstream log_file_;
    std::mt19937 rng_;

    // State
    uint32_t tx_seq_{0};
    uint32_t rx_seq_expected_{0};
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point last_command_time_;

    // Metrics
    std::atomic<uint64_t> telemetry_received_{0};
    std::atomic<uint64_t> commands_sent_{0};
    std::atomic<uint64_t> retries_{0};
    std::atomic<uint64_t> naks_sent_{0};
};
