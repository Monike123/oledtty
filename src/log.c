#include "log.h"

#include <stdarg.h>
#include <stdio.h>

static log_level_t g_log_level = LOG_LEVEL_INFO;

void log_set_level(log_level_t level) {
    g_log_level = level;
}

static void log_emit(log_level_t level, const char *tag, const char *fmt, va_list args) {
    if (level > g_log_level) {
        return;
    }
    fprintf(stderr, "[%s] ", tag);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
}

void log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_emit(LOG_LEVEL_ERROR, "ERROR", fmt, args);
    va_end(args);
}

void log_warn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_emit(LOG_LEVEL_WARN, "WARN ", fmt, args);
    va_end(args);
}

void log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_emit(LOG_LEVEL_INFO, "INFO ", fmt, args);
    va_end(args);
}

void log_debug(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_emit(LOG_LEVEL_DEBUG, "DEBUG", fmt, args);
    va_end(args);
}
