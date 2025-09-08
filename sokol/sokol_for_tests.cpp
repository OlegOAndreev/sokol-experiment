#include <sokol_log.h>

#include <cstdio>

// Override slog_func for tests
void slog_func(const char* tag, uint32_t log_level, uint32_t log_item, const char* message, uint32_t line_nr,
               const char* filename, void* user_data) {
    fprintf(stderr, "%s: %d: %d: %s: %s:%d\n", tag, log_level, log_item, message, filename, line_nr);
}
