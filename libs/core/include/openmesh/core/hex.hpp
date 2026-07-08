#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace openmesh::core {

// Lowercase hex encoding of a byte string. Handy for using a binary key (e.g. a
// public key) as a map key or log token.
inline std::string to_hex(const std::vector<std::uint8_t>& bytes) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (std::uint8_t b : bytes) {
        out.push_back(kHex[b >> 4]);
        out.push_back(kHex[b & 0x0F]);
    }
    return out;
}

} // namespace openmesh::core
