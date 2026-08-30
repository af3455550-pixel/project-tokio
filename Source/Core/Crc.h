#pragma once
// CRC32 (IEEE 802.3, reflected, poly 0xEDB88320) used for save-file integrity (§53).
#include <cstddef>
#include <cstdint>
#include <string>

namespace ink {

uint32_t Crc32(const uint8_t* data, std::size_t len);
uint32_t Crc32(const std::string& s);

} // namespace ink
