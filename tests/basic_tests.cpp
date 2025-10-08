#include "../include/crc.hpp"
#include "../include/packet.hpp"
#include "../include/thread_safe_queue.hpp"
#include "../include/link.hpp"
#include "../include/telemetry.hpp"
#include "../include/commands.hpp"
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>

// Simple test framework
int test_count = 0;
int test_passed = 0;

#define TEST(name) \
    void name(); \
    struct name##_register { \
        name##_register() { \
            std::cout << "Running test: " << #name << "..." << std::endl; \
            test_count++; \
            try { \
                name(); \
                test_passed++; \
                std::cout << "  ✓ PASSED" << std::endl; \
            } catch (const std::exception& e) { \
                std::cout << "  ✗ FAILED: " << e.what() << std::endl; \
            } \
        } \
    } name##_instance; \
    void name()

// Test CRC16 with known vectors
TEST(test_crc16_known_vectors) {
    // Test vector: "123456789" -> 0x29B1 (CRC-16/CCITT-FALSE)
    const uint8_t data[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    uint16_t result = crc::crc16_ccitt(data, sizeof(data));
    assert(result == 0x29B1);

    // Empty data
    uint16_t empty_result = crc::crc16_ccitt(nullptr, 0);
    assert(empty_result == 0xFFFF);  // Initial value
}

// Test ThreadSafeQueue basic operations
TEST(test_thread_safe_queue_basic) {
    ThreadSafeQueue<int> queue;

    queue.push(42);
    queue.push(100);

    auto val1 = queue.try_pop();
    assert(val1.has_value());
    assert(val1.value() == 42);

    auto val2 = queue.try_pop();
    assert(val2.has_value());
    assert(val2.value() == 100);

    auto val3 = queue.try_pop();
    assert(!val3.has_value());
}

// Test ThreadSafeQueue concurrency
TEST(test_thread_safe_queue_concurrency) {
    ThreadSafeQueue<int> queue;
    const int num_items = 1000;
    std::atomic<int> sum{0};

    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < num_items; ++i) {
            queue.push(i);
        }
    });

    // Consumer thread
    std::thread consumer([&]() {
        for (int i = 0; i < num_items; ++i) {
            int val = queue.pop();
            sum += val;
        }
    });

    producer.join();
    consumer.join();

    // Sum should be 0+1+2+...+(num_items-1) = num_items*(num_items-1)/2
    int expected_sum = num_items * (num_items - 1) / 2;
    assert(sum == expected_sum);
}

// Test Packet serialization/deserialization
TEST(test_packet_roundtrip) {
    Packet original;
    original.version = 1;
    original.type = PacketType::TelemetryPkt;
    original.seq = 12345;
    original.payload = "test payload data";
    original.payload_size = static_cast<uint32_t>(original.payload.size());
    original.compute_crc();

    // Serialize
    std::string bytes = original.to_bytes();

    // Deserialize
    Packet decoded = Packet::from_bytes(bytes);

    // Verify
    assert(decoded.version == original.version);
    assert(decoded.type == original.type);
    assert(decoded.seq == original.seq);
    assert(decoded.payload == original.payload);
    assert(decoded.crc16 == original.crc16);
    assert(decoded.verify_crc());
}

// Test CRC verification
TEST(test_packet_crc_verification) {
    Packet pkt;
    pkt.version = 1;
    pkt.type = PacketType::CommandPkt;
    pkt.seq = 100;
    pkt.payload = "command data";
    pkt.payload_size = static_cast<uint32_t>(pkt.payload.size());
    pkt.compute_crc();

    // Valid CRC
    assert(pkt.verify_crc());

    // Corrupt payload
    pkt.payload[0] = 'X';
    assert(!pkt.verify_crc());

    // Recompute CRC
    pkt.compute_crc();
    assert(pkt.verify_crc());
}

// Test Telemetry serialization
TEST(test_telemetry_serialization) {
    Telemetry t;
    t.ts = std::chrono::steady_clock::now();
    t.temperature_c = 65.5;
    t.battery_pct = 87.3;
    t.orbit_altitude_km = 405.2;
    t.pitch_deg = 1.5;
    t.yaw_deg = -0.3;
    t.roll_deg = 0.8;

    std::string serialized = t.to_json();
    Telemetry decoded = Telemetry::from_json(serialized);

    // Verify (with floating point tolerance)
    assert(decoded.temperature_c > 65.4 && decoded.temperature_c < 65.6);
    assert(decoded.battery_pct > 87.2 && decoded.battery_pct < 87.4);
    assert(decoded.orbit_altitude_km > 405.1 && decoded.orbit_altitude_km < 405.3);
}

// Test Command serialization
TEST(test_command_serialization) {
    Command cmd;
    cmd.type = CommandType::AdjustOrientation;
    cmd.d_pitch = 1.5;
    cmd.d_yaw = -0.5;
    cmd.d_roll = 0.2;

    std::string serialized = cmd.serialize();
    Command decoded = Command::deserialize(serialized);

    assert(decoded.type == cmd.type);
    assert(decoded.d_pitch == cmd.d_pitch);
    assert(decoded.d_yaw == cmd.d_yaw);
    assert(decoded.d_roll == cmd.d_roll);
}

// Test Link packet loss probability
TEST(test_link_loss_probability) {
    Link::Config config;
    config.latency_ms = 10;
    config.jitter_ms = 2;
    config.loss_prob = 0.5;  // 50% loss
    config.seed = 12345;

    Link link(config);

    const int num_packets = 1000;
    for (int i = 0; i < num_packets; ++i) {
        Packet pkt;
        pkt.type = PacketType::TelemetryPkt;
        pkt.seq = i;
        pkt.payload = "test";
        pkt.payload_size = 4;
        pkt.compute_crc();
        link.send_sat_to_gs(pkt);
    }

    // Wait for delivery
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Try to receive
    int received = 0;
    Packet pkt;
    while (link.recv_sat_to_gs(pkt, std::chrono::milliseconds(10))) {
        received++;
    }

    // Should be approximately 50% (allow 40-60% tolerance)
    double ratio = static_cast<double>(received) / num_packets;
    std::cout << "  Loss test: received " << received << "/" << num_packets
              << " (" << (ratio * 100.0) << "%)" << std::endl;
    assert(ratio > 0.35 && ratio < 0.65);
}

// Test end-to-end smoke test
TEST(test_end_to_end_smoke) {
    Link::Config link_config;
    link_config.latency_ms = 10;
    link_config.jitter_ms = 2;
    link_config.loss_prob = 0.0;  // No loss for smoke test
    link_config.seed = 999;

    Link link(link_config);

    // Send telemetry packet from satellite
    Packet telem_pkt;
    telem_pkt.type = PacketType::TelemetryPkt;
    telem_pkt.seq = 1;
    telem_pkt.payload = "ts=1000|temp=50.0|batt=90.0|alt=400.0|pitch=0.0|yaw=0.0|roll=0.0";
    telem_pkt.payload_size = static_cast<uint32_t>(telem_pkt.payload.size());
    telem_pkt.compute_crc();
    link.send_sat_to_gs(telem_pkt);

    // Ground station receives and sends ACK
    Packet received_pkt;
    bool got_packet = link.recv_sat_to_gs(received_pkt, std::chrono::milliseconds(100));
    assert(got_packet);
    assert(received_pkt.type == PacketType::TelemetryPkt);
    assert(received_pkt.verify_crc());

    Packet ack_pkt;
    ack_pkt.type = PacketType::AckPkt;
    ack_pkt.seq = 1;
    ack_pkt.payload = "";
    ack_pkt.payload_size = 0;
    ack_pkt.compute_crc();
    link.send_gs_to_sat(ack_pkt);

    // Satellite receives ACK
    Packet received_ack;
    bool got_ack = link.recv_gs_to_sat(received_ack, std::chrono::milliseconds(100));
    assert(got_ack);
    assert(received_ack.type == PacketType::AckPkt);
    assert(received_ack.seq == 1);

    std::cout << "  End-to-end smoke test: ACK successfully received" << std::endl;
}

int main() {
    std::cout << "\n=== Running Satellite Simulator Tests ===" << std::endl;
    std::cout << "\nTest results:" << std::endl;
    std::cout << "\n" << test_passed << "/" << test_count << " tests passed" << std::endl;

    if (test_passed == test_count) {
        std::cout << "\n✓ All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n✗ Some tests failed!" << std::endl;
        return 1;
    }
}
