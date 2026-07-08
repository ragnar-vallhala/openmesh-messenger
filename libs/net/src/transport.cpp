#include "openmesh/net/endpoint.hpp"

// Placeholder translation unit for the Networking Engine.
//
// TODO: implement the UDP socket layer, the direct/NAT/relay connection
// strategy (SRS FR-7), signaling- and relay-client logic, reconnection and
// retransmission. Kept as a stub so the library builds and can be linked.

namespace openmesh::net {

const char* connect_path_name(ConnectPath path) {
    switch (path) {
    case ConnectPath::Direct:       return "direct";
    case ConnectPath::NatTraversal: return "nat-traversal";
    case ConnectPath::Relay:        return "relay";
    }
    return "unknown";
}

} // namespace openmesh::net
