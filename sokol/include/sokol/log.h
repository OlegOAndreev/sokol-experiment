#pragma once

#include <sokol_log.h>

#include <cstdarg>
#include <cstdio>

// Convenient wrapper to format string and log info or error using slog_func from sokol. Usage: SLOG_INFO("mah format
// %d", 123)
#define SLOG_INFO(...) _my_slog_impl(3, __FILE__, __LINE__, __VA_ARGS__)
#define SLOG_ERROR(...) _my_slog_impl(1, __FILE__, __LINE__, __VA_ARGS__)

#if defined(__clang__) || defined(__GNUC__)
#define _MY_SLOG_FMT_ARGS(FORMAT_IDX) __attribute__((format(printf, FORMAT_IDX, FORMAT_IDX + 1)))
#else
#define _MY_SLOG_FMT_ARGS(FORMAT_IDX)
#endif
_MY_SLOG_FMT_ARGS(4) inline void _my_slog_impl(int log_level, const char* filename, int line_nr, const char* fmt, ...) {
    char buffer[10000];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    slog_func(nullptr, log_level, 0, buffer, line_nr, filename, nullptr);
}
