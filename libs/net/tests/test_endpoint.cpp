#include "openmesh/net/endpoint.hpp"

#include <cassert>

int main() {
    using namespace openmesh::net;
    Endpoint ep{"192.0.2.1", 4433};
    assert(ep.to_string() == "192.0.2.1:4433");
    return 0;
}
