#include "openmesh/crypto/identity.hpp"
#include "openmesh/storage/identity_store.hpp"
#include "openmesh/storage/store.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace openmesh::storage;
namespace fs = std::filesystem;

namespace {
using Bytes = std::vector<std::uint8_t>;

std::string temp_path(const char* name) {
    const auto p = fs::temp_directory_path() / name;
    std::error_code ec;
    fs::remove(p, ec);
    return p.string();
}

Contact make(const Bytes& pk, const std::string& nick, TrustStatus trust, std::int64_t seen) {
    Contact c;
    c.public_key = pk;
    c.nickname = nick;
    c.trust = trust;
    c.last_seen = seen;
    return c;
}

void flip_last_byte(const std::string& path) {
    std::fstream f(path, std::ios::binary | std::ios::in | std::ios::out | std::ios::ate);
    assert(f);
    const auto size = f.tellp();
    f.seekg(static_cast<std::streamoff>(size) - 1);
    char b = 0;
    f.read(&b, 1);
    b = static_cast<char>(b ^ 0xFF);
    f.seekp(static_cast<std::streamoff>(size) - 1);
    f.write(&b, 1);
}
} // namespace

static void test_contacts_persist() {
    const std::string path = temp_path("om_test_contacts.omdb");
    const Bytes alice = {0x01, 0x02, 0x03};
    const Bytes bob = {0x0A, 0x0B};

    {
        Store store;
        assert(store.open(path, "correct horse battery"));
        assert(store.is_persistent());
        store.upsert(make(alice, "alice", TrustStatus::Accepted, 111)); // auto-saved
        store.upsert(make(bob, "bob", TrustStatus::Pending, 222));
    }

    // Reopen in a fresh Store -> contacts survive.
    {
        Store store;
        assert(store.open(path, "correct horse battery"));
        assert(store.contacts().size() == 2);
        assert(store.trust_of(alice) == TrustStatus::Accepted);
        auto b = store.find(bob);
        assert(b.has_value() && b->nickname == "bob" && b->last_seen == 222);
    }

    // Wrong passphrase is rejected.
    {
        Store store;
        assert(!store.open(path, "wrong passphrase"));
        assert(!store.is_persistent());
    }

    // Tampering with the file breaks authentication.
    flip_last_byte(path);
    {
        Store store;
        assert(!store.open(path, "correct horse battery"));
    }

    std::error_code ec;
    fs::remove(path, ec);
}

static void test_identity_persist() {
    const std::string path = temp_path("om_test_identity.omid");
    const auto id = openmesh::crypto::generate_identity();

    assert(save_identity(path, id, "s3cret"));

    auto loaded = load_identity(path, "s3cret");
    assert(loaded.has_value());
    assert(loaded->public_key == id.public_key);
    assert(loaded->secret_key == id.secret_key);
    assert(loaded->fingerprint() == id.fingerprint());

    assert(!load_identity(path, "nope").has_value());
    assert(!load_identity(temp_path("om_missing.omid"), "s3cret").has_value());

    std::error_code ec;
    fs::remove(path, ec);
}

int main() {
    test_contacts_persist();
    test_identity_persist();
    return 0;
}
