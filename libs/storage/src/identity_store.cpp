#include "openmesh/storage/identity_store.hpp"

#include "blob.hpp"
#include "openmesh/crypto/kdf.hpp"

#include <array>

namespace openmesh::storage {
namespace {
constexpr std::array<char, 4> kMagic = {'O', 'M', 'I', 'D'};
} // namespace

bool save_identity(const std::string& path, const crypto::Identity& identity,
                   const std::string& passphrase) {
    if (!identity.valid()) {
        return false;
    }
    const auto salt = crypto::generate_salt();
    const auto key = crypto::derive_key(passphrase, salt);
    if (key.empty()) {
        return false;
    }
    detail::Bytes plaintext;
    detail::put_bytes(plaintext, identity.public_key);
    detail::put_bytes(plaintext, identity.secret_key);
    return detail::write_blob(path, kMagic, salt, key, plaintext);
}

std::optional<crypto::Identity> load_identity(const std::string& path,
                                              const std::string& passphrase) {
    auto salt = detail::read_salt(path, kMagic);
    if (!salt) {
        return std::nullopt;
    }
    const auto key = crypto::derive_key(passphrase, *salt);
    if (key.empty()) {
        return std::nullopt;
    }
    auto plaintext = detail::read_blob(path, kMagic, key);
    if (!plaintext) {
        return std::nullopt; // wrong passphrase or tampered
    }

    detail::Reader r(*plaintext);
    crypto::Identity id;
    if (!r.bytes(id.public_key) || !r.bytes(id.secret_key) || !r.done() || !id.valid()) {
        return std::nullopt;
    }
    return id;
}

} // namespace openmesh::storage
