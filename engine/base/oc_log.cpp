#include "engine/base/oc_log.hpp"

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <mutex>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace oc::log {
namespace {

std::mutex g_mutex;
Level g_min_level = Level::debug;
bool g_colors_enabled = false;

void enable_ansi_colors_once() {
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE) return;
    DWORD mode = 0;
    if (!GetConsoleMode(handle, &mode)) return; // redirected output: no colors
    if (SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        g_colors_enabled = true;
    }
}

const char* level_tag(Level level) {
    switch (level) {
        case Level::debug: return "dbg";
        case Level::info: return "inf";
        case Level::warn: return "wrn";
        case Level::error: return "err";
    }
    return "???";
}

const char* level_color(Level level) {
    switch (level) {
        case Level::debug: return "\x1b[90m";
        case Level::info: return "\x1b[32m";
        case Level::warn: return "\x1b[33m";
        case Level::error: return "\x1b[31m";
    }
    return "";
}

} // namespace

void set_min_level(Level level) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_min_level = level;
}

void write(Level level, std::string_view msg) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (static_cast<int>(level) < static_cast<int>(g_min_level)) return;

    static bool colors_once = [] { enable_ansi_colors_once(); return true; }();
    (void)colors_once;

    SYSTEMTIME st;
    GetLocalTime(&st);

    std::FILE* out = stdout;
    if (g_colors_enabled) {
        std::fprintf(out, "%s%02u:%02u:%02u.%03u [%s]\x1b[0m %.*s\n", level_color(level), st.wHour, st.wMinute,
            st.wSecond, st.wMilliseconds, level_tag(level), static_cast<int>(msg.size()), msg.data());
    } else {
        std::fprintf(out, "%02u:%02u:%02u.%03u [%s] %.*s\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
            level_tag(level), static_cast<int>(msg.size()), msg.data());
    }
    std::fflush(out);
}

void debugf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buffer[2048];
    int n = std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if (n <= 0) return;
    write(Level::debug, std::string_view(buffer, static_cast<size_t>(n)));
}

void infof(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buffer[2048];
    int n = std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if (n <= 0) return;
    write(Level::info, std::string_view(buffer, static_cast<size_t>(n)));
}

void warnf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buffer[2048];
    int n = std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if (n <= 0) return;
    write(Level::warn, std::string_view(buffer, static_cast<size_t>(n)));
}

void errorf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buffer[2048];
    int n = std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if (n <= 0) return;
    write(Level::error, std::string_view(buffer, static_cast<size_t>(n)));
}

} // namespace oc::log
