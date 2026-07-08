#include "openmesh/storage/store.hpp"

#include "blob.hpp"
#include "openmesh/crypto/kdf.hpp"

#include <array>
#include <filesystem>
#include <utility>

namespace openmesh::storage {
namespace {

constexpr std::array<char, 4> kMagic = {'O', 'M', 'D', 'B'};

detail::Bytes encode(const std::vector<Contact>& contacts) {
    detail::Bytes out;
    detail::put_u32(out, static_cast<std::uint32_t>(contacts.size()));
    for (const auto& c : contacts) {
        detail::put_bytes(out, c.public_key);
        detail::put_str(out, c.nickname);
        detail::put_u8(out, static_cast<std::uint8_t>(c.trust));
        detail::put_u64(out, static_cast<std::uint64_t>(c.last_seen));
    }
    return out;
}

bool decode(const detail::Bytes& blob, std::vector<Contact>& out) {
    detail::Reader r(blob);
    std::uint32_t count = 0;
    if (!r.u32(count)) {
        return false;
    }
    std::vector<Contact> loaded;
    loaded.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        Contact c;
        std::uint8_t trust = 0;
        std::uint64_t last_seen = 0;
        if (!r.bytes(c.public_key) || !r.str(c.nickname) || !r.u8(trust) || !r.u64(last_seen)) {
            return false;
        }
        c.trust = static_cast<TrustStatus>(trust);
        c.last_seen = static_cast<std::int64_t>(last_seen);
        loaded.push_back(std::move(c));
    }
    if (!r.done()) {
        return false; // trailing bytes -> corrupt
    }
    out = std::move(loaded);
    return true;
}

} // namespace

bool Store::open(const std::string& path, const std::string& passphrase) {
    if (std::filesystem::exists(path)) {
        auto salt = detail::read_salt(path, kMagic);
        if (!salt) {
            return false;
        }
        auto key = crypto::derive_key(passphrase, *salt);
        if (key.empty()) {
            return false;
        }
        auto plaintext = detail::read_blob(path, kMagic, key);
        if (!plaintext) {
            return false; // wrong passphrase or tampered
        }
        std::vector<Contact> loaded;
        if (!decode(*plaintext, loaded)) {
            return false;
        }
        contacts_ = std::move(loaded);
        salt_ = std::move(*salt);
        key_ = std::move(key);
        path_ = path;
        persistent_ = true;
        return true;
    }

    // New store: fresh salt + derived key, then create the file.
    auto salt = crypto::generate_salt();
    auto key = crypto::derive_key(passphrase, salt);
    if (key.empty()) {
        return false;
    }
    salt_ = std::move(salt);
    key_ = std::move(key);
    path_ = path;
    persistent_ = true;
    return save();
}

bool Store::save() const {
    if (!persistent_) {
        return false;
    }
    return detail::write_blob(path_, kMagic, salt_, key_, encode(contacts_));
}

void Store::add_contact(const Contact& contact) {
    contacts_.push_back(contact);
    if (persistent_) {
        save();
    }
}

void Store::upsert(const Contact& contact) {
    for (auto& c : contacts_) {
        if (c.public_key == contact.public_key) {
            c = contact;
            if (persistent_) {
                save();
            }
            return;
        }
    }
    contacts_.push_back(contact);
    if (persistent_) {
        save();
    }
}

std::vector<Contact> Store::contacts() const {
    return contacts_;
}

std::optional<Contact> Store::find(const std::vector<std::uint8_t>& public_key) const {
    for (const auto& c : contacts_) {
        if (c.public_key == public_key) {
            return c;
        }
    }
    return std::nullopt;
}

std::optional<TrustStatus> Store::trust_of(const std::vector<std::uint8_t>& public_key) const {
    for (const auto& c : contacts_) {
        if (c.public_key == public_key) {
            return c.trust;
        }
    }
    return std::nullopt;
}

} // namespace openmesh::storage
