#ifndef LOGGING_H
#define LOGGING_H

#define DEBUG(...) log(__FILE__, __LINE__, __func__, "DEBUG", ##__VA_ARGS__)
#define INFO(...) log(__FILE__, __LINE__, __func__, "INFO", ##__VA_ARGS__)
#define WARNING(...) log(__FILE__, __LINE__, __func__, "WARNING", ##__VA_ARGS__)
#define ERROR(...) log(__FILE__, __LINE__, __func__, "ERROR", ##__VA_ARGS__)
#define FATAL(...) log(__FILE__, __LINE__, __func__, "FATAL", ##__VA_ARGS__)

#endif