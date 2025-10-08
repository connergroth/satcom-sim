#pragma once

#include <cstdint>
#include <cstddef>

/**
 * CRC-16/CCITT-FALSE (polynomial 0x1021, init 0xFFFF, no reflection)
 * Used for packet integrity verification in satellite communications.
 */
namespace crc {

/**
 * Compute CRC-16/CCITT-FALSE checksum over data buffer.
 *
 * @param data Pointer to data buffer
 * @param len Length of data in bytes
 * @return 16-bit CRC checksum
 */
uint16_t crc16_ccitt(const uint8_t* data, size_t len);

} // namespace crc
