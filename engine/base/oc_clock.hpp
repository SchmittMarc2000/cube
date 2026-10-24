#pragma once

#include <chrono>
#include <cstdint>

namespace oc {

// Monotonic clock, safe for timing (frame times, RTTs, tick deltas).
inline uint64_t now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

inline double ns_to_ms(uint64_t ns) { return static_cast<double>(ns) / 1.0e6; }
inline double ns_to_s(uint64_t ns) { return static_cast<double>(ns) / 1.0e9; }

} // namespace oc
