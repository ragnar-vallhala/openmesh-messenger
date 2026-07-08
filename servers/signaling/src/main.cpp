#include "openmesh/core/logging.hpp"
#include "openmesh/core/version.hpp"
#include "server.hpp"

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <string>

// Entry point for the volunteer signaling server (SRS FR-6, §11).
//
// Usage: openmesh-signaling [bind_host] [bind_port]   (default 0.0.0.0:4433)
//
// TODO: parse config/signaling.example.toml instead of positional args.
namespace {
std::atomic<openmesh::signaling::SignalingServer*> g_server{nullptr};

void on_signal(int) {
    if (auto* s = g_server.load()) {
        s->stop();
    }
}
} // namespace

int main(int argc, char** argv) {
    std::string host = "0.0.0.0";
    std::uint16_t port = 4433;
    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = static_cast<std::uint16_t>(std::strtoul(argv[2], nullptr, 10));
    }

    openmesh::signaling::SignalingServer server;
    if (!server.bind({host, port})) {
        openmesh::core::log_error("signaling: failed to bind " + host + ":" + std::to_string(port));
        return 1;
    }

    g_server.store(&server);
    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    const auto local = server.local_endpoint();
    openmesh::core::log_info(std::string("openmesh-signaling ") + openmesh::core::kVersionString +
                             " listening on " + (local ? local->to_string() : "?"));

    server.run();

    openmesh::core::log_info("signaling: shutting down");
    return 0;
}
