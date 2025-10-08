#include "link.hpp"
#include <algorithm>

Link::Link(const Config& config)
    : config_(config), rng_(config.seed) {}

void Link::send_sat_to_gs(Packet pkt) {
    apply_impairments_and_send(std::move(pkt), sat_to_gs_);
}

bool Link::recv_sat_to_gs(Packet& out, std::chrono::milliseconds timeout) {
    auto opt = sat_to_gs_.try_pop(timeout);
    if (opt) {
        out = std::move(*opt);
        return true;
    }
    return false;
}

void Link::send_gs_to_sat(Packet pkt) {
    apply_impairments_and_send(std::move(pkt), gs_to_sat_);
}

bool Link::recv_gs_to_sat(Packet& out, std::chrono::milliseconds timeout) {
    auto opt = gs_to_sat_.try_pop(timeout);
    if (opt) {
        out = std::move(*opt);
        return true;
    }
    return false;
}

void Link::apply_impairments_and_send(Packet pkt, ThreadSafeQueue<Packet>& queue) {
    packets_sent_++;

    // Thread-safe random number generation
    double loss_value, delay_ms;
    {
        std::lock_guard<std::mutex> lock(rng_mutex_);

        // Apply packet loss
        std::uniform_real_distribution<double> loss_dist(0.0, 1.0);
        loss_value = loss_dist(rng_);

        // Apply latency with jitter (normal distribution)
        std::normal_distribution<double> latency_dist(
            static_cast<double>(config_.latency_ms),
            static_cast<double>(config_.jitter_ms)
        );
        delay_ms = std::max(0.0, latency_dist(rng_));
    }

    if (loss_value < config_.loss_prob) {
        packets_dropped_++;
        return;  // Packet lost
    }

    // Sleep to simulate latency (inline, simple approach)
    if (delay_ms > 0) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(static_cast<long long>(delay_ms))
        );
    }

    // Enqueue packet
    queue.push(std::move(pkt));
}
