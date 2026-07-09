#pragma once

#include "openmesh/crypto/identity.hpp"
#include "openmesh/net/endpoint.hpp"

#include <optional>
#include <string>

namespace omchat {

// Run the full-screen ncurses UI. Blocks until the user quits. Returns 0 on a
// clean exit. `store_path`/`store_pass` enable encrypted persistence when set.
int run_tui(openmesh::crypto::Identity self, std::optional<openmesh::net::Endpoint> server,
            std::optional<openmesh::net::Endpoint> relay, std::string display_name,
            std::string store_path, std::string store_pass);

} // namespace omchat
