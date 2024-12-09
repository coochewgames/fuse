#include <stdio.h>

#include "logging.h"


void log(const char *filename, unsigned int line, const char *function, const char *level, const char *message, ...) {
    va_list args;
    va_start(args, message);

    if (strcmp(level, "DEBUG") == 0) {
        fprintf(stdout, "[%s] %s:%d:%s - ", level, filename, line, function);
    } else {
        fprintf(stderr, "[%s] %s - ", level, function);
    }

    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");

    va_end(args);
}