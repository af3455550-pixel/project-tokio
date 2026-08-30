#include "Core/Crc.h"

namespace ink {

namespace {
constexpr uint32_t kPoly = 0xEDB88320u;

const uint32_t* Table() {
    static uint32_t table[256] = {};
    static bool built = false;
    if (!built) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k)
                c = (c & 1u) ? (kPoly ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        built = true;
    }
    return table;
}
} // namespace

uint32_t Crc32(const uint8_t* data, std::size_t len) {
    const uint32_t* t = Table();
    uint32_t c = 0xFFFFFFFFu;
    for (std::size_t i = 0; i < len; ++i)
        c = t[(c ^ data[i]) & 0xFFu] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

uint32_t Crc32(const std::string& s) { return Crc32(reinterpret_cast<const uint8_t*>(s.data()), s.size()); }

} // namespace ink
