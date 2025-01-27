#ifndef LOGGING_H
#define LOGGING_H

#include <stdarg.h>

typedef enum {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
} LOG_LEVEL;

#define DEBUG(...) log_message(__FILE__, __LINE__, __func__, "DEBUG", ##__VA_ARGS__)
#define INFO(...) log_message(__FILE__, __LINE__, __func__, "INFO", ##__VA_ARGS__)
#define WARNING(...) log_message(__FILE__, __LINE__, __func__, "WARNING", ##__VA_ARGS__)
#define ERROR(...) log_message(__FILE__, __LINE__, __func__, "ERROR", ##__VA_ARGS__)
#define FATAL(...) log_message(__FILE__, __LINE__, __func__, "FATAL", ##__VA_ARGS__)


void set_log_level(LOG_LEVEL level);
void log_message(const char *filename, unsigned int line, const char *function, const char *level, const char *message, ...);

#endif
