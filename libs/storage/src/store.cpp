#include "openmesh/storage/store.hpp"

namespace openmesh::storage {

void Store::add_contact(const Contact& contact) {
    contacts_.push_back(contact);
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
