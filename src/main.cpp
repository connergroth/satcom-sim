#include "satellite.hpp"
#include "ground_station.hpp"
#include "link.hpp"
#include <iostream>
#include <string>
#include <cstring>
#include <iomanip>

struct SimConfig {
    int duration_sec = 20;
    double telemetry_rate_hz = 5.0;
    double loss = 0.05;
    int latency_ms = 100;
    int jitter_ms = 30;
    int ack_timeout_ms = 150;
    int max_retries = 3;
    unsigned int seed = 42;
    std::string log_file = "telemetry.log";
    bool verbose = false;
    bool help = false;
};

void print_help(const char* prog_name) {
    std::cout << "Satellite Telemetry & Command Simulator\n\n"
              << "Usage: " << prog_name << " [options]\n\n"
              << "Options:\n"
              << "  --duration-sec N       Simulation duration in seconds (default: 20)\n"
              << "  --telemetry-rate-hz F  Telemetry emission rate in Hz (default: 5.0)\n"
              << "  --loss F               Packet loss probability 0..1 (default: 0.05)\n"
              << "  --latency-ms N         Mean link latency in ms (default: 100)\n"
              << "  --jitter-ms N          Latency jitter (std dev) in ms (default: 30)\n"
              << "  --ack-timeout-ms N     ACK timeout in ms (default: 150)\n"
              << "  --max-retries N        Maximum retry attempts (default: 3)\n"
              << "  --seed N               Random seed for determinism (default: 42)\n"
              << "  --log-file PATH        Telemetry log file path (default: telemetry.log)\n"
              << "  --verbose              Enable verbose logging\n"
              << "  --help                 Show this help message\n";
}

bool parse_args(int argc, char* argv[], SimConfig& config) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help") {
            config.help = true;
            return true;
        } else if (arg == "--verbose") {
            config.verbose = true;
        } else if (arg == "--duration-sec" && i + 1 < argc) {
            config.duration_sec = std::atoi(argv[++i]);
        } else if (arg == "--telemetry-rate-hz" && i + 1 < argc) {
            config.telemetry_rate_hz = std::atof(argv[++i]);
        } else if (arg == "--loss" && i + 1 < argc) {
            config.loss = std::atof(argv[++i]);
        } else if (arg == "--latency-ms" && i + 1 < argc) {
            config.latency_ms = std::atoi(argv[++i]);
        } else if (arg == "--jitter-ms" && i + 1 < argc) {
            config.jitter_ms = std::atoi(argv[++i]);
        } else if (arg == "--ack-timeout-ms" && i + 1 < argc) {
            config.ack_timeout_ms = std::atoi(argv[++i]);
        } else if (arg == "--max-retries" && i + 1 < argc) {
            config.max_retries = std::atoi(argv[++i]);
        } else if (arg == "--seed" && i + 1 < argc) {
            config.seed = static_cast<unsigned int>(std::atoi(argv[++i]));
        } else if (arg == "--log-file" && i + 1 < argc) {
            config.log_file = argv[++i];
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return false;
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    SimConfig sim_config;

    if (!parse_args(argc, argv, sim_config)) {
        print_help(argv[0]);
        return 1;
    }

    if (sim_config.help) {
        print_help(argv[0]);
        return 0;
    }

    std::cout << "=== Satellite Telemetry & Command Simulator ===" << std::endl;
    std::cout << "Duration: " << sim_config.duration_sec << "s" << std::endl;
    std::cout << "Telemetry rate: " << sim_config.telemetry_rate_hz << " Hz" << std::endl;
    std::cout << "Loss probability: " << (sim_config.loss * 100.0) << "%" << std::endl;
    std::cout << "Link latency: " << sim_config.latency_ms << "ms ± " << sim_config.jitter_ms << "ms" << std::endl;
    std::cout << "ACK timeout: " << sim_config.ack_timeout_ms << "ms" << std::endl;
    std::cout << "Max retries: " << sim_config.max_retries << std::endl;
    std::cout << "Random seed: " << sim_config.seed << std::endl;
    std::cout << "Log file: " << sim_config.log_file << std::endl;
    std::cout << "Verbose: " << (sim_config.verbose ? "yes" : "no") << std::endl;
    std::cout << "===============================================\n" << std::endl;

    // Create link
    Link::Config link_config;
    link_config.latency_ms = sim_config.latency_ms;
    link_config.jitter_ms = sim_config.jitter_ms;
    link_config.loss_prob = sim_config.loss;
    link_config.seed = sim_config.seed;
    Link link(link_config);

    // Create satellite
    Satellite::Config sat_config;
    sat_config.telemetry_rate_hz = sim_config.telemetry_rate_hz;
    sat_config.ack_timeout_ms = sim_config.ack_timeout_ms;
    sat_config.max_retries = sim_config.max_retries;
    sat_config.verbose = sim_config.verbose;
    sat_config.seed = sim_config.seed;
    Satellite satellite(link, sat_config);

    // Create ground station
    GroundStation::Config gs_config;
    gs_config.ack_timeout_ms = sim_config.ack_timeout_ms;
    gs_config.max_retries = sim_config.max_retries;
    gs_config.log_file = sim_config.log_file;
    gs_config.verbose = sim_config.verbose;
    gs_config.seed = sim_config.seed;
    GroundStation ground_station(link, gs_config);

    // Start simulation
    std::cout << "Starting simulation..." << std::endl;
    satellite.start();
    ground_station.start();

    // Run for specified duration
    std::this_thread::sleep_for(std::chrono::seconds(sim_config.duration_sec));

    // Stop simulation
    std::cout << "\nStopping simulation..." << std::endl;
    satellite.stop();
    ground_station.stop();

    // Print metrics
    std::cout << "\n=== Simulation Metrics ===" << std::endl;
    std::cout << "Satellite:" << std::endl;
    std::cout << "  Telemetry sent: " << satellite.get_telemetry_sent() << std::endl;
    std::cout << "  Commands received: " << satellite.get_commands_received() << std::endl;
    std::cout << "  Retries: " << satellite.get_retries() << std::endl;
    std::cout << "  NAKs received: " << satellite.get_naks_received() << std::endl;
    std::cout << "\nGround Station:" << std::endl;
    std::cout << "  Telemetry received: " << ground_station.get_telemetry_received() << std::endl;
    std::cout << "  Commands sent: " << ground_station.get_commands_sent() << std::endl;
    std::cout << "  Retries: " << ground_station.get_retries() << std::endl;
    std::cout << "  NAKs sent: " << ground_station.get_naks_sent() << std::endl;
    std::cout << "\nLink:" << std::endl;
    std::cout << "  Packets sent: " << link.get_packets_sent() << std::endl;
    std::cout << "  Packets dropped: " << link.get_packets_dropped() << std::endl;
    std::cout << "  Drop rate: " << std::fixed << std::setprecision(2)
              << (100.0 * link.get_packets_dropped() / std::max(1ULL, link.get_packets_sent())) << "%"
              << std::endl;
    std::cout << "==========================\n" << std::endl;

    std::cout << "Telemetry logged to: " << sim_config.log_file << std::endl;

    return 0;
}
