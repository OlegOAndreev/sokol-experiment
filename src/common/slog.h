#pragma once

#include <sokol_log.h>

#include <cstdarg>
#include <cstdio>

#include "common.h"
#include "thread_name.h"


// Convenient wrapper to format string and log info or error using slog_func from sokol. Usage: SLOG_INFO("mah format
// %d", 123)
#define SLOG_INFO(...) _my_slog_impl(3, __FILE__, __LINE__, __VA_ARGS__)
#define SLOG_ERROR(...) _my_slog_impl(1, __FILE__, __LINE__, __VA_ARGS__)

#if defined(__clang__) || defined(__GNUC__)
#define _MY_SLOG_FMT_ARGS(FORMAT_IDX) __attribute__((format(printf, FORMAT_IDX, FORMAT_IDX + 1)))
#else
#define _MY_SLOG_FMT_ARGS(FORMAT_IDX)
#endif
NO_INLINE _MY_SLOG_FMT_ARGS(4) inline void _my_slog_impl(int log_level, const char* filename, int line_nr,
                                                         const char* fmt, ...) {
    char tag_buffer[1000];
    const char* tag = nullptr;
    char buffer[10000];
    size_t offset = 0;
    if (const char* thread_name = local_thread_pool_name(); thread_name != nullptr) {
        snprintf(tag_buffer, sizeof(tag_buffer), "%s:%zu", thread_name, local_thread_pool_worker_id());
        tag = tag_buffer;
    }
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer + offset, sizeof(buffer) - offset, fmt, args);
    va_end(args);
    slog_func(tag, log_level, 0, buffer, line_nr, filename, nullptr);
}
