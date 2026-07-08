#pragma once

#include "openmesh/storage/contact.hpp"

#include <optional>
#include <string>
#include <vector>

namespace openmesh::storage {

// Facade over the local, device-owned contact database (SRS FR-3, FR-9).
//
// In-memory by default. Call open() to make it durable: the contents are
// serialized and encrypted at rest (Argon2id-derived key + XChaCha20-Poly1305)
// under a user passphrase (SRS §13). After open(), mutations auto-persist. A
// SQLite backend could replace this behind the same facade later.
class Store {
public:
    // Enable encrypted, file-backed persistence at `path`, protected by
    // `passphrase`. Loads the file if it exists (empty store otherwise). Returns
    // false on a wrong passphrase, corrupt/tampered file, or I/O error.
    bool open(const std::string& path, const std::string& passphrase);

    // Write the current state to disk. Called automatically after mutations once
    // persistence is enabled. Returns false if not persistent or on I/O error.
    bool save() const;

    [[nodiscard]] bool is_persistent() const { return persistent_; }

    void add_contact(const Contact& contact);

    // Insert a contact, or replace the existing one with the same public key.
    void upsert(const Contact& contact);

    [[nodiscard]] std::vector<Contact> contacts() const;
    [[nodiscard]] std::optional<Contact> find(const std::vector<std::uint8_t>& public_key) const;

    // Trust status of a contact, or nullopt if unknown (never seen).
    [[nodiscard]] std::optional<TrustStatus>
    trust_of(const std::vector<std::uint8_t>& public_key) const;

private:
    std::vector<Contact> contacts_;

    // Persistence state (set by open()). The key is derived once at open() so
    // auto-saves are cheap (AEAD only, no repeated Argon2id).
    bool persistent_ = false;
    std::string path_;
    std::vector<std::uint8_t> key_;
    std::vector<std::uint8_t> salt_;
};

} // namespace openmesh::storage
