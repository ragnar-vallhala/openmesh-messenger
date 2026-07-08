#include "openmesh/core/logging.hpp"

#include <iostream>

namespace openmesh::core {

namespace {
const char* to_string(LogLevel level) {
    switch (level) {
    case LogLevel::Trace: return "TRACE";
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info:  return "INFO";
    case LogLevel::Warn:  return "WARN";
    case LogLevel::Error: return "ERROR";
    }
    return "?";
}
} // namespace

void log(LogLevel level, std::string_view message) {
    std::cerr << '[' << to_string(level) << "] " << message << '\n';
}

} // namespace openmesh::core
