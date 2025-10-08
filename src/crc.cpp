#include "crc.hpp"

namespace crc {

/**
 * CRC-16/CCITT-FALSE implementation.
 * Polynomial: 0x1021, Init: 0xFFFF, No XOR out, No reflection.
 */
uint16_t crc16_ccitt(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < len; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;

        for (int j = 0; j < 8; ++j) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }

    return crc;
}

} // namespace crc
