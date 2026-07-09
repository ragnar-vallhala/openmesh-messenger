#include "openmesh/core/logging.hpp"
#include "openmesh/core/version.hpp"
#include "server.hpp"

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <string>

// Entry point for the optional relay server (SRS FR-8, §12).
//
// Usage: openmesh-relay [bind_host] [bind_port]   (default 0.0.0.0:4434)
namespace {
std::atomic<openmesh::relay::RelayServer*> g_server{nullptr};

void on_signal(int) {
    if (auto* s = g_server.load()) {
        s->stop();
    }
}
} // namespace

int main(int argc, char** argv) {
    std::string host = "0.0.0.0";
    std::uint16_t port = 4434;
    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = static_cast<std::uint16_t>(std::strtoul(argv[2], nullptr, 10));
    }

    openmesh::relay::RelayServer server;
    if (!server.bind({host, port})) {
        openmesh::core::log_error("relay: failed to bind " + host + ":" + std::to_string(port));
        return 1;
    }

    g_server.store(&server);
    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    const auto local = server.local_endpoint();
    openmesh::core::log_info(std::string("openmesh-relay ") + openmesh::core::kVersionString +
                             " listening on " + (local ? local->to_string() : "?"));

    server.run();

    openmesh::core::log_info("relay: shutting down");
    return 0;
}
