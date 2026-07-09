#include "router.hpp"

#include <cassert>

int main() {
    using namespace openmesh::relay;

    RouteTable routes;
    routes.upsert("alice", {"192.0.2.1", 5000}, /*now=*/100);
    routes.upsert("bob", {"192.0.2.2", 6000}, /*now=*/100);
    assert(routes.size() == 2);

    auto a = routes.lookup("alice");
    assert(a && a->port == 5000);
    assert(!routes.lookup("carol").has_value());

    // Refreshing keeps a route alive past expiry.
    routes.upsert("alice", {"192.0.2.1", 5000}, /*now=*/200);
    routes.expire(/*cutoff=*/150); // drops entries last seen before 150
    assert(routes.lookup("alice").has_value());
    assert(!routes.lookup("bob").has_value());
    assert(routes.size() == 1);
    return 0;
}
