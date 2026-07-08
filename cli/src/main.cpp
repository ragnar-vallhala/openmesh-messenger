// om-chat — a minimal command-line client for OpenMesh Messenger.
//
// Registers with a signaling server, discovers peers by public key, runs the
// contact-request flow, and exchanges end-to-end encrypted messages. Networking
// runs on a background thread that owns the Engine; the main thread only reads
// stdin and hands lines to it, so the Engine is touched from one thread only.

#include "openmesh/core/hex.hpp"
#include "openmesh/crypto/identity.hpp"
#include "openmesh/crypto/sign.hpp"
#include "openmesh/messaging/engine.hpp"
#include "openmesh/net/endpoint.hpp"
#include "openmesh/net/signaling_client.hpp"
#include "openmesh/storage/identity_store.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using openmesh::crypto::Identity;
using openmesh::messaging::Bytes;
using openmesh::messaging::Engine;
using openmesh::messaging::Message;
using openmesh::net::Endpoint;
using openmesh::net::SignalingClient;
using openmesh::storage::TrustStatus;

namespace {

Bytes to_bytes(const std::string& s) {
    return Bytes(s.begin(), s.end());
}
std::string to_string(const Bytes& b) {
    return std::string(b.begin(), b.end());
}

std::optional<Bytes> from_hex(const std::string& hex) {
    if (hex.size() % 2 != 0) {
        return std::nullopt;
    }
    Bytes out;
    out.reserve(hex.size() / 2);
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        return -1;
    };
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        const int hi = nibble(hex[i]);
        const int lo = nibble(hex[i + 1]);
        if (hi < 0 || lo < 0) {
            return std::nullopt;
        }
        out.push_back(static_cast<std::uint8_t>((hi << 4) | lo));
    }
    return out;
}

std::optional<Endpoint> parse_endpoint(const std::string& s) {
    const auto pos = s.rfind(':');
    if (pos == std::string::npos || pos + 1 >= s.size()) {
        return std::nullopt;
    }
    Endpoint ep;
    ep.host = s.substr(0, pos);
    ep.port = static_cast<std::uint16_t>(std::strtoul(s.c_str() + pos + 1, nullptr, 10));
    return ep;
}

SignalingClient::Signer signer_for(const Bytes& secret_key) {
    return [secret_key](const Bytes& challenge) {
        return openmesh::crypto::sign_detached(challenge, secret_key);
    };
}

class ChatApp {
public:
    ChatApp(Identity id, Endpoint server, std::string display_name)
        : self_(std::move(id)), display_name_(std::move(display_name)),
          engine_(self_.public_key, self_.secret_key),
          sig_(engine_.transport(), server, self_.public_key, signer_for(self_.secret_key)) {}

    void submit(const std::string& line) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(line);
    }
    void stop() { running_ = false; }
    [[nodiscard]] bool running() const { return running_; }

    void run() {
        engine_.on_message([this](const Message& m) {
            std::cout << "\n[" << name_of(m.peer) << "] " << to_string(m.plaintext) << "\n> "
                      << std::flush;
        });
        engine_.on_contact_request([this](const Bytes& peer, const Bytes& greeting) {
            const std::string name = auto_alias(peer);
            std::cout << "\n* contact request from " << name;
            if (!greeting.empty()) {
                std::cout << " (\"" << to_string(greeting) << "\")";
            }
            std::cout << " — /accept " << name << " or /reject " << name << "\n> " << std::flush;
        });
        engine_.on_contact_response([this](const Bytes& peer, bool accepted) {
            std::cout << "\n* " << name_of(peer) << (accepted ? " accepted" : " declined")
                      << " your request\n> " << std::flush;
        });

        if (!engine_.bind(Endpoint{"0.0.0.0", 0})) {
            std::cout << "error: could not bind a local socket\n";
            running_ = false;
            return;
        }
        std::cout << "Registering with the signaling server...\n";
        if (sig_.register_self()) {
            std::cout << "Registered. You are online.\n";
        } else {
            std::cout << "Registration failed (is the server reachable?). You can retry with "
                         "/register.\n";
        }
        print_help();
        std::cout << "> " << std::flush;

        auto last_heartbeat = std::chrono::steady_clock::now();
        while (running_) {
            std::deque<std::string> lines;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                lines.swap(queue_);
            }
            for (const auto& line : lines) {
                handle(line);
            }
            if (!running_) {
                break;
            }
            engine_.poll(/*timeout_ms=*/50);
            const auto now = std::chrono::steady_clock::now();
            if (now - last_heartbeat > std::chrono::seconds(30)) {
                sig_.heartbeat();
                last_heartbeat = now;
            }
        }
    }

private:
    void print_help() {
        std::cout << "Commands:\n"
                     "  /id                       show your public key (share it) and fingerprint\n"
                     "  /add <name> <pubkeyhex>   discover a peer and send a contact request\n"
                     "  /accept <name>            accept an incoming contact request\n"
                     "  /reject <name>            reject an incoming contact request\n"
                     "  /msg <name> <text>        send a message to an accepted contact\n"
                     "  <text>                    send to the current peer (last one you used)\n"
                     "  /peers                    list known peers and trust status\n"
                     "  /register                 (re)register with the signaling server\n"
                     "  /help                     show this help\n"
                     "  /quit                     exit\n";
    }

    std::string hex(const Bytes& pk) const { return openmesh::core::to_hex(pk); }

    std::string name_of(const Bytes& pk) {
        auto it = pk_to_name_.find(hex(pk));
        if (it != pk_to_name_.end()) {
            return it->second;
        }
        return hex(pk).substr(0, 8);
    }

    // Ensure an incoming/unknown peer has a local name (its fingerprint prefix).
    std::string auto_alias(const Bytes& pk) {
        const std::string key = hex(pk);
        auto it = pk_to_name_.find(key);
        if (it != pk_to_name_.end()) {
            return it->second;
        }
        const std::string name = key.substr(0, 8);
        name_to_pk_[name] = pk;
        pk_to_name_[key] = name;
        return name;
    }

    std::optional<Bytes> resolve(const std::string& token) {
        auto it = name_to_pk_.find(token);
        if (it != name_to_pk_.end()) {
            return it->second;
        }
        if (auto pk = from_hex(token); pk && pk->size() == 32) {
            return pk;
        }
        return std::nullopt;
    }

    void handle(const std::string& line) {
        if (line.empty()) {
            return;
        }
        if (line[0] != '/') {
            send_current(line);
            return;
        }
        std::istringstream in(line);
        std::string cmd;
        in >> cmd;
        if (cmd == "/quit") {
            running_ = false;
        } else if (cmd == "/help") {
            print_help();
        } else if (cmd == "/id") {
            std::cout << "public key: " << hex(self_.public_key) << "\n"
                      << "fingerprint: " << self_.fingerprint() << "\n";
        } else if (cmd == "/register") {
            std::cout << (sig_.register_self() ? "Registered.\n" : "Registration failed.\n");
        } else if (cmd == "/add") {
            cmd_add(in);
        } else if (cmd == "/accept" || cmd == "/reject") {
            cmd_decide(in, cmd == "/accept");
        } else if (cmd == "/msg") {
            cmd_msg(in);
        } else if (cmd == "/peers") {
            cmd_peers();
        } else {
            std::cout << "unknown command: " << cmd << " (try /help)\n";
        }
        std::cout << "> " << std::flush;
    }

    void cmd_add(std::istringstream& in) {
        std::string name, pkhex;
        in >> name >> pkhex;
        auto pk = from_hex(pkhex);
        if (name.empty() || !pk || pk->size() != 32) {
            std::cout << "usage: /add <name> <64-hex-char public key>\n";
            return;
        }
        name_to_pk_[name] = *pk;
        pk_to_name_[hex(*pk)] = name;
        current_ = *pk;

        std::cout << "Looking up " << name << "...\n";
        auto found = sig_.discover(*pk);
        if (!found.endpoint) {
            std::cout << (found.responded ? "Peer is offline (not registered).\n"
                                          : "No response from the signaling server.\n");
            return;
        }
        std::cout << "Found at " << found.endpoint->to_string() << ". Sending contact request...\n";
        if (engine_.send_contact_request(*pk, *found.endpoint, to_bytes(display_name_))) {
            std::cout << "Request sent. Waiting for " << name << " to accept.\n";
        } else {
            std::cout << "Could not send the request.\n";
        }
    }

    void cmd_decide(std::istringstream& in, bool accept) {
        std::string name;
        in >> name;
        auto pk = resolve(name);
        if (!pk) {
            std::cout << "unknown peer: " << name << "\n";
            return;
        }
        const bool ok = accept ? engine_.accept_contact(*pk) : engine_.reject_contact(*pk);
        if (ok) {
            current_ = *pk;
            std::cout << (accept ? "Accepted " : "Rejected ") << name << ".\n";
        } else {
            std::cout << "No pending request from " << name << ".\n";
        }
    }

    void cmd_msg(std::istringstream& in) {
        std::string name;
        in >> name;
        std::string text;
        std::getline(in, text);
        if (!text.empty() && text[0] == ' ') {
            text.erase(0, 1);
        }
        auto pk = resolve(name);
        if (!pk) {
            std::cout << "unknown peer: " << name << "\n";
            return;
        }
        current_ = *pk;
        deliver(*pk, text);
    }

    void send_current(const std::string& text) {
        if (current_.empty()) {
            std::cout << "no current peer — use /msg <name> <text> first\n";
            return;
        }
        deliver(current_, text);
    }

    void deliver(const Bytes& pk, const std::string& text) {
        if (!engine_.send(pk, to_bytes(text))) {
            std::cout << "not delivered — " << name_of(pk)
                      << " is not an accepted contact (or unreachable)\n";
        }
    }

    void cmd_peers() {
        if (name_to_pk_.empty()) {
            std::cout << "no known peers yet\n";
            return;
        }
        for (const auto& [name, pk] : name_to_pk_) {
            const auto trust = engine_.trust_of(pk);
            std::string status = "unknown";
            if (trust == TrustStatus::Accepted)
                status = "accepted";
            else if (trust == TrustStatus::Pending)
                status = "pending";
            else if (trust == TrustStatus::Rejected)
                status = "rejected";
            std::cout << "  " << name << "  " << status << "  " << hex(pk).substr(0, 16) << "…\n";
        }
    }

    Identity self_;
    std::string display_name_;
    Engine engine_;
    SignalingClient sig_;

    std::mutex mutex_;
    std::deque<std::string> queue_;
    std::atomic<bool> running_{true};

    // All CLI state below is owned by the network thread only.
    std::unordered_map<std::string, Bytes> name_to_pk_;
    std::unordered_map<std::string, std::string> pk_to_name_;
    Bytes current_;
};

void usage() {
    std::cout << "usage: om-chat --server <host:port> [--identity <file> --pass <passphrase>] "
                 "[--name <display name>]\n";
}

} // namespace

int main(int argc, char** argv) {
    std::string server, identity_file, passphrase, display_name;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&]() -> std::string { return (i + 1 < argc) ? argv[++i] : std::string(); };
        if (a == "--server")
            server = next();
        else if (a == "--identity")
            identity_file = next();
        else if (a == "--pass")
            passphrase = next();
        else if (a == "--name")
            display_name = next();
        else if (a == "-h" || a == "--help") {
            usage();
            return 0;
        } else {
            std::cout << "unknown argument: " << a << "\n";
            usage();
            return 1;
        }
    }
    if (passphrase.empty()) {
        if (const char* env = std::getenv("OM_PASS")) {
            passphrase = env;
        }
    }

    auto server_ep = parse_endpoint(server);
    if (!server_ep) {
        std::cout << "error: --server <host:port> is required\n";
        usage();
        return 1;
    }

    // Load or create the identity.
    Identity id;
    if (!identity_file.empty()) {
        if (passphrase.empty()) {
            std::cout << "error: --pass (or OM_PASS) is required with --identity\n";
            return 1;
        }
        if (std::filesystem::exists(identity_file)) {
            auto loaded = openmesh::storage::load_identity(identity_file, passphrase);
            if (!loaded) {
                std::cout << "error: wrong passphrase or corrupt identity file\n";
                return 1;
            }
            id = *loaded;
            std::cout << "Loaded identity from " << identity_file << "\n";
        } else {
            id = openmesh::crypto::generate_identity();
            if (!openmesh::storage::save_identity(identity_file, id, passphrase)) {
                std::cout << "error: could not save identity to " << identity_file << "\n";
                return 1;
            }
            std::cout << "Created a new identity, saved to " << identity_file << "\n";
        }
    } else {
        id = openmesh::crypto::generate_identity();
        std::cout << "Using an ephemeral identity (not saved). Use --identity to keep one.\n";
    }

    std::cout << "Your fingerprint: " << id.fingerprint() << "\n";

    ChatApp app(std::move(id), *server_ep, display_name);
    std::thread net([&app] { app.run(); });

    std::string line;
    while (app.running() && std::getline(std::cin, line)) {
        if (line == "/quit") {
            app.stop();
            break;
        }
        app.submit(line);
    }
    app.stop();
    net.join();
    std::cout << "\nBye.\n";
    return 0;
}
