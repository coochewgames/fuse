#include <stdio.h>
#include <string.h>

#include "logging.h"


static LOG_LEVEL min_level_to_log = INFO;

static const char *get_log_level_name(LOG_LEVEL level);


void set_log_level(LOG_LEVEL level) {
    min_level_to_log = level;
}

void log_message(const char *filename, unsigned int line, const char *function, LOG_LEVEL level, const char *message, ...) {
    if (level < min_level_to_log) {
        return;
    }

    va_list args;
    va_start(args, message);

    if (level == DEBUG) {
        fprintf(stderr, "[%s] %s() - ", get_log_level_name(level), function);
    } else {
        fprintf(stderr, "[%s] %s:%d - ", get_log_level_name(level), filename, line);
    }

    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");

    va_end(args);
}

static const char *get_log_level_name(LOG_LEVEL level) {
    switch (level) {
        case DEBUG:
            return "DEBUG";
        case INFO:
            return "INFO";
        case WARNING:
            return "WARNING";
        case ERROR:
            return "ERROR";
        case FATAL:
            return "FATAL";
        default:
            return "UNKNOWN";
    }
}
