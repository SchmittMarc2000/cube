#pragma once

#include <cstdint>
#include <string_view>

namespace oc::log {

enum class Level : int { debug, info, warn, error };

// Thread-safe console sink with timestamps. Colored on terminals that support it.
void write(Level level, std::string_view msg);

void set_min_level(Level level);

inline void debug(std::string_view msg) { write(Level::debug, msg); }
inline void info(std::string_view msg) { write(Level::info, msg); }
inline void warn(std::string_view msg) { write(Level::warn, msg); }
inline void error(std::string_view msg) { write(Level::error, msg); }

// printf-style convenience (format + args).
void debugf(const char* fmt, ...);
void infof(const char* fmt, ...);
void warnf(const char* fmt, ...);
void errorf(const char* fmt, ...);

} // namespace oc::log
