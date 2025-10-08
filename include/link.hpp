#pragma once

#include "packet.hpp"
#include "thread_safe_queue.hpp"
#include <chrono>
#include <random>
#include <thread>
#include <memory>

/**
 * Simulated bidirectional radio link between satellite and ground station.
 * Introduces realistic impairments: latency, jitter, and packet loss.
 */
class Link {
public:
    /**
     * Configuration for link impairments.
     */
    struct Config {
        int latency_ms = 100;      // Mean latency
        int jitter_ms = 30;        // Latency jitter (std deviation)
        double loss_prob = 0.05;   // Packet loss probability [0..1]
        unsigned int seed = 42;    // Random seed for determinism
    };

    explicit Link(const Config& config);

    // Satellite → Ground Station
    void send_sat_to_gs(Packet pkt);
    bool recv_sat_to_gs(Packet& out, std::chrono::milliseconds timeout);

    // Ground Station → Satellite
    void send_gs_to_sat(Packet pkt);
    bool recv_gs_to_sat(Packet& out, std::chrono::milliseconds timeout);

    // Metrics
    uint64_t get_packets_dropped() const { return packets_dropped_; }
    uint64_t get_packets_sent() const { return packets_sent_; }

private:
    // Apply latency and loss, then enqueue with delay
    void apply_impairments_and_send(Packet pkt, ThreadSafeQueue<Packet>& queue);

    Config config_;
    std::mt19937 rng_;
    std::mutex rng_mutex_;  // Protect RNG for thread safety

    // Queues for each direction
    ThreadSafeQueue<Packet> sat_to_gs_;
    ThreadSafeQueue<Packet> gs_to_sat_;

    // Metrics
    std::atomic<uint64_t> packets_dropped_{0};
    std::atomic<uint64_t> packets_sent_{0};
};
