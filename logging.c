#include <stdio.h>
#include <string.h>

#include "logging.h"


#ifdef DEBUG
static LOG_LEVEL log_level = DEBUG;
#else
static LOG_LEVEL log_level = INFO;
#endif

void set_log_level(LOG_LEVEL level) {
    log_level = level;
}

void log_message(const char *filename, unsigned int line, const char *function, const char *level, const char *message, ...) {
    va_list args;
    va_start(args, message);

    if (strcmp(level, "DEBUG") == 0) {
        fprintf(stderr, "[%s] %s() - ", level, function);
    } else {
        fprintf(stderr, "[%s] %s:%d - ", level, filename, line);
    }

    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");

    va_end(args);
}
