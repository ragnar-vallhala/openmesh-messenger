#pragma once

// Internal helpers for the on-disk, at-rest-encrypted storage files (SRS FR-9,
// §13). Not part of the public storage API.
//
// File layout:  [magic:4][version:1][salt:16][ sealed = nonce||ciphertext||tag ]
// The header (magic+version+salt) is bound as AEAD associated data, so tampering
// with it fails decryption. The 32-byte key is derived by the caller from a
// passphrase + the salt (crypto::derive_key).

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace openmesh::storage::detail {

using Bytes = std::vector<std::uint8_t>;

inline constexpr std::uint8_t kFileVersion = 1;
inline constexpr std::size_t kSaltBytes = 16;

// ---- big-endian serialization helpers --------------------------------------
inline void put_u8(Bytes& o, std::uint8_t v) {
    o.push_back(v);
}
inline void put_u16(Bytes& o, std::uint16_t v) {
    o.push_back(static_cast<std::uint8_t>(v >> 8));
    o.push_back(static_cast<std::uint8_t>(v & 0xFF));
}
inline void put_u32(Bytes& o, std::uint32_t v) {
    for (int s = 24; s >= 0; s -= 8) {
        o.push_back(static_cast<std::uint8_t>((v >> s) & 0xFF));
    }
}
inline void put_u64(Bytes& o, std::uint64_t v) {
    for (int s = 56; s >= 0; s -= 8) {
        o.push_back(static_cast<std::uint8_t>((v >> s) & 0xFF));
    }
}
inline void put_bytes(Bytes& o, const Bytes& b) {
    put_u16(o, static_cast<std::uint16_t>(b.size()));
    o.insert(o.end(), b.begin(), b.end());
}
inline void put_str(Bytes& o, const std::string& s) {
    put_u16(o, static_cast<std::uint16_t>(s.size()));
    o.insert(o.end(), s.begin(), s.end());
}

// Bounds-checked cursor reader.
class Reader {
public:
    explicit Reader(const Bytes& b) : b_(b) {}

    bool u8(std::uint8_t& v) {
        if (off_ + 1 > b_.size()) {
            return false;
        }
        v = b_[off_++];
        return true;
    }
    bool u16(std::uint16_t& v) {
        if (off_ + 2 > b_.size()) {
            return false;
        }
        v = static_cast<std::uint16_t>((b_[off_] << 8) | b_[off_ + 1]);
        off_ += 2;
        return true;
    }
    bool u32(std::uint32_t& v) {
        if (off_ + 4 > b_.size()) {
            return false;
        }
        v = 0;
        for (int i = 0; i < 4; ++i) {
            v = (v << 8) | b_[off_++];
        }
        return true;
    }
    bool u64(std::uint64_t& v) {
        if (off_ + 8 > b_.size()) {
            return false;
        }
        v = 0;
        for (int i = 0; i < 8; ++i) {
            v = (v << 8) | static_cast<std::uint64_t>(b_[off_++]);
        }
        return true;
    }
    bool bytes(Bytes& out) {
        std::uint16_t n = 0;
        if (!u16(n) || off_ + n > b_.size()) {
            return false;
        }
        out.assign(b_.begin() + static_cast<std::ptrdiff_t>(off_),
                   b_.begin() + static_cast<std::ptrdiff_t>(off_ + n));
        off_ += n;
        return true;
    }
    bool str(std::string& out) {
        std::uint16_t n = 0;
        if (!u16(n) || off_ + n > b_.size()) {
            return false;
        }
        out.assign(b_.begin() + static_cast<std::ptrdiff_t>(off_),
                   b_.begin() + static_cast<std::ptrdiff_t>(off_ + n));
        off_ += n;
        return true;
    }
    [[nodiscard]] bool done() const { return off_ == b_.size(); }

private:
    const Bytes& b_;
    std::size_t off_ = 0;
};

// ---- encrypted-file container ----------------------------------------------
// Read just the salt from an existing file's header (for key derivation).
std::optional<Bytes> read_salt(const std::string& path, const std::array<char, 4>& magic);

// Encrypt `plaintext` with `key` and atomically write the file. `salt` (16 B) is
// stored in the header; `key` must be the 32-byte key derived from it.
bool write_blob(const std::string& path, const std::array<char, 4>& magic, const Bytes& salt,
                const Bytes& key, const Bytes& plaintext);

// Read and decrypt a file written by write_blob. Returns nullopt on missing file,
// bad magic/version, or failed authentication (wrong key / tampered).
std::optional<Bytes> read_blob(const std::string& path, const std::array<char, 4>& magic,
                               const Bytes& key);

} // namespace openmesh::storage::detail
