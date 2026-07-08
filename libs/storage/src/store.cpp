#include "openmesh/storage/store.hpp"

namespace openmesh::storage {

void Store::add_contact(const Contact& contact) {
    contacts_.push_back(contact);
}

void Store::upsert(const Contact& contact) {
    for (auto& c : contacts_) {
        if (c.public_key == contact.public_key) {
            c = contact;
            return;
        }
    }
    contacts_.push_back(contact);
}

std::optional<TrustStatus> Store::trust_of(const std::vector<std::uint8_t>& public_key) const {
    for (const auto& c : contacts_) {
        if (c.public_key == public_key) {
            return c.trust;
        }
    }
    return std::nullopt;
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

} // namespace openmesh::storage
