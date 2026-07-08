#include "openmesh/storage/store.hpp"

#include <cassert>

int main() {
    using namespace openmesh::storage;

    Store store;
    Contact c;
    c.public_key = {0x01, 0x02, 0x03};
    c.nickname = "alice";
    c.trust = TrustStatus::Accepted;
    store.add_contact(c);

    assert(store.contacts().size() == 1);
    assert(store.find({0x01, 0x02, 0x03}).has_value());
    assert(!store.find({0xFF}).has_value());
    return 0;
}
