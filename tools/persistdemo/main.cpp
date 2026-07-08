#include "openmesh/core/hex.hpp"
#include "openmesh/crypto/identity.hpp"
#include "openmesh/storage/identity_store.hpp"
#include "openmesh/storage/store.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace openmesh;
namespace fs = std::filesystem;

namespace {
using Bytes = std::vector<std::uint8_t>;

storage::Contact contact(const Bytes& pk, const std::string& nick, storage::TrustStatus t) {
    storage::Contact c;
    c.public_key = pk;
    c.nickname = nick;
    c.trust = t;
    return c;
}

Bytes read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return Bytes(std::istreambuf_iterator<char>(in), {});
}

bool contains_text(const Bytes& data, const std::string& needle) {
    const std::string hay(data.begin(), data.end());
    return hay.find(needle) != std::string::npos;
}
} // namespace

// Demonstrates at-rest encrypted persistence (SRS FR-9, §13): contacts and the
// identity survive a "restart", but the files on disk are opaque ciphertext.
int main() {
    const std::string pass = "correct horse battery staple";
    const std::string db = (fs::temp_directory_path() / "om_persist_demo.omdb").string();
    const std::string idfile = (fs::temp_directory_path() / "om_persist_demo.omid").string();
    std::error_code ec;
    fs::remove(db, ec);
    fs::remove(idfile, ec);

    const auto identity = crypto::generate_identity();
    const Bytes alice_pk = {0xA1, 0xA2, 0xA3};

    std::cout << "=== First run ===\n";
    storage::save_identity(idfile, identity, pass);
    {
        storage::Store store;
        store.open(db, pass);
        store.upsert(contact(alice_pk, "alice", storage::TrustStatus::Accepted));
        store.upsert(contact({0xB1}, "bob", storage::TrustStatus::Pending));
        std::cout << "Saved identity " << identity.fingerprint().substr(0, 16) << "… and "
                  << store.contacts().size() << " contacts.\n";
    }

    const Bytes on_disk = read_file(db);
    std::cout << "\nContacts file on disk (" << on_disk.size() << " bytes), first 48:\n  "
              << core::to_hex(Bytes(on_disk.begin(),
                                    on_disk.begin() + std::min<std::size_t>(48, on_disk.size())))
              << "\n";
    std::cout << "Does the nickname \"alice\" appear in the file? "
              << (contains_text(on_disk, "alice") ? "YES (bug!)" : "no — it's encrypted") << "\n";

    std::cout << "\n=== Second run (simulated restart) ===\n";
    {
        storage::Store store;
        if (!store.open(db, pass)) {
            std::cout << "failed to open store\n";
            return 1;
        }
        std::cout << "Loaded " << store.contacts().size() << " contacts:";
        for (const auto& c : store.contacts()) {
            std::cout << " " << c.nickname;
        }
        std::cout << "\n";
        auto loaded = storage::load_identity(idfile, pass);
        std::cout << "Identity restored, fingerprint matches: "
                  << (loaded && loaded->fingerprint() == identity.fingerprint() ? "yes" : "no")
                  << "\n";
    }

    std::cout << "\nWrong passphrase opens the store? ";
    storage::Store wrong;
    std::cout << (wrong.open(db, "guessing") ? "YES (bug!)" : "no — rejected") << "\n";

    fs::remove(db, ec);
    fs::remove(idfile, ec);
    return 0;
}
