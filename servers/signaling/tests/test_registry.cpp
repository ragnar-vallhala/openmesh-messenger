#include "registry.hpp"

#include <cassert>

int main() {
    using namespace openmesh::signaling;

    PeerRegistry registry;
    registry.upsert("alicefp", {"192.0.2.1", 4433}, /*now=*/100);
    registry.upsert("bobfp", {"192.0.2.2", 4433}, /*now=*/100);
    assert(registry.size() == 2);

    auto ep = registry.lookup("alicefp");
    assert(ep.has_value());
    assert(ep->port == 4433);
    assert(!registry.lookup("nobody").has_value());

    // Expire everything last seen before t=200.
    registry.expire(/*cutoff=*/200);
    assert(registry.size() == 0);
    return 0;
}
