#include "openmesh/core/logging.hpp"
#include "openmesh/core/version.hpp"

#include <cassert>
#include <string_view>

int main() {
    // Smoke test: logging must not throw and version must be populated.
    openmesh::core::log_info("core test running");
    assert(std::string_view{openmesh::core::kVersionString} == "0.1.0");
    return 0;
}
