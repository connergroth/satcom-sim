#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <cstring>

/**
 * Packet types for satellite-ground station communication protocol.
 */
enum class PacketType : uint8_t {
    TelemetryPkt = 1,
    CommandPkt = 2,
    AckPkt = 3,
    NakPkt = 4
};

/**
 * Network packet structure with header, payload, and CRC footer.
 * Used for all communication between satellite and ground station.
 */
struct Packet {
    // Header fields
    uint16_t version = 1;
    PacketType type;
    uint32_t seq;
    uint32_t payload_size;

    // Payload
    std::string payload;

    // Footer
    uint16_t crc16;

    /**
     * Serialize packet to byte string for transmission.
     * Format: [version:2][type:1][seq:4][payload_size:4][payload:N][crc:2]
     */
    std::string to_bytes() const;

    /**
     * Deserialize packet from byte string.
     * Throws std::runtime_error if malformed.
     */
    static Packet from_bytes(const std::string& bytes);

    /**
     * Compute CRC over header + payload and update crc16 field.
     */
    void compute_crc();

    /**
     * Verify CRC matches computed value.
     */
    bool verify_crc() const;

    /**
     * Get human-readable packet type name.
     */
    std::string type_name() const {
        switch (type) {
            case PacketType::TelemetryPkt: return "Telemetry";
            case PacketType::CommandPkt: return "Command";
            case PacketType::AckPkt: return "ACK";
            case PacketType::NakPkt: return "NAK";
            default: return "Unknown";
        }
    }
};
