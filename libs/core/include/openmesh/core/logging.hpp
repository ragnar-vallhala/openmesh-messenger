#pragma once

#include <string_view>

namespace openmesh::core {

enum class LogLevel { Trace, Debug, Info, Warn, Error };

// Minimal logging facade. The backend is intentionally trivial for now
// (writes to stderr); it can later be swapped for a structured/async sink
// without touching call sites.
void log(LogLevel level, std::string_view message);

inline void log_info(std::string_view msg) { log(LogLevel::Info, msg); }
inline void log_warn(std::string_view msg) { log(LogLevel::Warn, msg); }
inline void log_error(std::string_view msg) { log(LogLevel::Error, msg); }
inline void log_debug(std::string_view msg) { log(LogLevel::Debug, msg); }

} // namespace openmesh::core
