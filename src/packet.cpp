#include "packet.hpp"
#include "crc.hpp"
#include <stdexcept>
#include <cstring>

std::string Packet::to_bytes() const {
    std::string bytes;
    bytes.reserve(11 + payload.size() + 2);  // header(11) + payload + crc(2)

    // Version (2 bytes, big-endian)
    bytes.push_back(static_cast<char>(version >> 8));
    bytes.push_back(static_cast<char>(version & 0xFF));

    // Type (1 byte)
    bytes.push_back(static_cast<char>(type));

    // Sequence (4 bytes, big-endian)
    bytes.push_back(static_cast<char>(seq >> 24));
    bytes.push_back(static_cast<char>((seq >> 16) & 0xFF));
    bytes.push_back(static_cast<char>((seq >> 8) & 0xFF));
    bytes.push_back(static_cast<char>(seq & 0xFF));

    // Payload size (4 bytes, big-endian)
    uint32_t psize = static_cast<uint32_t>(payload.size());
    bytes.push_back(static_cast<char>(psize >> 24));
    bytes.push_back(static_cast<char>((psize >> 16) & 0xFF));
    bytes.push_back(static_cast<char>((psize >> 8) & 0xFF));
    bytes.push_back(static_cast<char>(psize & 0xFF));

    // Payload
    bytes.append(payload);

    // CRC (2 bytes, big-endian)
    bytes.push_back(static_cast<char>(crc16 >> 8));
    bytes.push_back(static_cast<char>(crc16 & 0xFF));

    return bytes;
}

Packet Packet::from_bytes(const std::string& bytes) {
    if (bytes.size() < 13) {  // Minimum: header(11) + crc(2)
        throw std::runtime_error("Packet too short");
    }

    Packet pkt;
    size_t pos = 0;

    // Version
    pkt.version = (static_cast<uint8_t>(bytes[pos]) << 8) |
                  static_cast<uint8_t>(bytes[pos + 1]);
    pos += 2;

    // Type
    pkt.type = static_cast<PacketType>(bytes[pos++]);

    // Sequence
    pkt.seq = (static_cast<uint32_t>(static_cast<uint8_t>(bytes[pos])) << 24) |
              (static_cast<uint32_t>(static_cast<uint8_t>(bytes[pos + 1])) << 16) |
              (static_cast<uint32_t>(static_cast<uint8_t>(bytes[pos + 2])) << 8) |
              static_cast<uint32_t>(static_cast<uint8_t>(bytes[pos + 3]));
    pos += 4;

    // Payload size
    pkt.payload_size = (static_cast<uint32_t>(static_cast<uint8_t>(bytes[pos])) << 24) |
                       (static_cast<uint32_t>(static_cast<uint8_t>(bytes[pos + 1])) << 16) |
                       (static_cast<uint32_t>(static_cast<uint8_t>(bytes[pos + 2])) << 8) |
                       static_cast<uint32_t>(static_cast<uint8_t>(bytes[pos + 3]));
    pos += 4;

    // Validate payload size
    if (bytes.size() < pos + pkt.payload_size + 2) {
        throw std::runtime_error("Payload size mismatch");
    }

    // Payload
    pkt.payload = bytes.substr(pos, pkt.payload_size);
    pos += pkt.payload_size;

    // CRC
    pkt.crc16 = (static_cast<uint8_t>(bytes[pos]) << 8) |
                static_cast<uint8_t>(bytes[pos + 1]);

    return pkt;
}

void Packet::compute_crc() {
    // Build header + payload for CRC computation
    std::string data;
    data.reserve(11 + payload.size());

    data.push_back(static_cast<char>(version >> 8));
    data.push_back(static_cast<char>(version & 0xFF));
    data.push_back(static_cast<char>(type));
    data.push_back(static_cast<char>(seq >> 24));
    data.push_back(static_cast<char>((seq >> 16) & 0xFF));
    data.push_back(static_cast<char>((seq >> 8) & 0xFF));
    data.push_back(static_cast<char>(seq & 0xFF));

    uint32_t psize = static_cast<uint32_t>(payload.size());
    data.push_back(static_cast<char>(psize >> 24));
    data.push_back(static_cast<char>((psize >> 16) & 0xFF));
    data.push_back(static_cast<char>((psize >> 8) & 0xFF));
    data.push_back(static_cast<char>(psize & 0xFF));
    data.append(payload);

    crc16 = crc::crc16_ccitt(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

bool Packet::verify_crc() const {
    std::string data;
    data.reserve(11 + payload.size());

    data.push_back(static_cast<char>(version >> 8));
    data.push_back(static_cast<char>(version & 0xFF));
    data.push_back(static_cast<char>(type));
    data.push_back(static_cast<char>(seq >> 24));
    data.push_back(static_cast<char>((seq >> 16) & 0xFF));
    data.push_back(static_cast<char>((seq >> 8) & 0xFF));
    data.push_back(static_cast<char>(seq & 0xFF));

    uint32_t psize = static_cast<uint32_t>(payload.size());
    data.push_back(static_cast<char>(psize >> 24));
    data.push_back(static_cast<char>((psize >> 16) & 0xFF));
    data.push_back(static_cast<char>((psize >> 8) & 0xFF));
    data.push_back(static_cast<char>(psize & 0xFF));
    data.append(payload);

    uint16_t computed = crc::crc16_ccitt(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    return computed == crc16;
}
