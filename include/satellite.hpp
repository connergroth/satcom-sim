#pragma once

#include "link.hpp"
#include "telemetry.hpp"
#include "commands.hpp"
#include "packet.hpp"
#include <atomic>
#include <thread>
#include <random>

/**
 * Satellite simulator running in its own thread.
 * Periodically emits telemetry and responds to ground station commands.
 */
class Satellite {
public:
    struct Config {
        double telemetry_rate_hz = 5.0;
        int ack_timeout_ms = 150;
        int max_retries = 3;
        bool verbose = false;
        unsigned int seed = 42;
    };

    Satellite(Link& link, const Config& config);
    ~Satellite();

    // Start/stop satellite thread
    void start();
    void stop();

    // Metrics
    uint64_t get_telemetry_sent() const { return telemetry_sent_; }
    uint64_t get_commands_received() const { return commands_received_; }
    uint64_t get_retries() const { return retries_; }
    uint64_t get_naks_received() const { return naks_received_; }

private:
    void run();
    void send_telemetry();
    void process_commands();
    void update_state(double dt);
    void check_anomalies();
    bool wait_for_ack(uint32_t seq, std::chrono::milliseconds timeout);

    Link& link_;
    Config config_;
    std::atomic<bool> running_{false};
    std::thread thread_;
    std::mt19937 rng_;

    // State
    uint32_t tx_seq_{0};
    uint32_t rx_seq_expected_{0};
    bool safe_mode_{false};

    // Telemetry state
    double temperature_c_{50.0};
    double battery_pct_{90.0};
    double orbit_altitude_km_{400.0};
    double pitch_deg_{0.0};
    double yaw_deg_{0.0};
    double roll_deg_{0.0};

    // Metrics
    std::atomic<uint64_t> telemetry_sent_{0};
    std::atomic<uint64_t> commands_received_{0};
    std::atomic<uint64_t> retries_{0};
    std::atomic<uint64_t> naks_received_{0};
};
