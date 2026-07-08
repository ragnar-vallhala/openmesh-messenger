#pragma once

#include "openmesh/storage/contact.hpp"

#include <optional>
#include <vector>

namespace openmesh::storage {

// Facade over the local, device-owned data store (SRS FR-9).
//
// This is an in-memory skeleton so higher layers can be developed against a
// stable interface. Replace the implementation with a persistent, at-rest
// encrypted backend (e.g. SQLite).
class Store {
public:
    void add_contact(const Contact& contact);
    [[nodiscard]] std::vector<Contact> contacts() const;
    [[nodiscard]] std::optional<Contact> find(const std::vector<std::uint8_t>& public_key) const;

private:
    std::vector<Contact> contacts_;
};

} // namespace openmesh::storage
