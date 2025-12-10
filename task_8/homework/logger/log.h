#ifndef _HLOGGER
#define _HLOGGER

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum LogLevel {
    LOGL_DEBUG = 100,
    LOGL_INFO  = 200,
    LOGL_WARN  = 250,
    LOGL_ERROR = 300
} LogLevel;

typedef enum OutputMode {
    COLOR_MODE   = 10,
    DEFAULT_MODE = 1
} OutputMode;

typedef struct Logger       Logger;
typedef struct ServiceLines ServiceLines;

bool shouldLog(LogLevel levelMsg);

Logger*         GetLogger(void);
ServiceLines*   GetServiceLines(void);

int  LoggerInit(LogLevel levelLogger, const char *log_file_name, OutputMode color_mode);
void LoggerDeinit(void);

const char* ColorLogMsg(LogLevel levelMsg);

void RemoveAnsiCodes(char *str);
void log_begin(LogLevel levelMsg, const char *file, size_t line, const char *func, const char *fmt, ...);
void log_msg();
void log_end();

#define LOG(levelMsg, fmt, ...)                   \
    do {                                          \
        LOG_BEGIN(levelMsg, fmt, ##__VA_ARGS__);  \
        LOG_MSG();                                \
        LOG_END();                                \
    } while(0)

#define LOG_BEGIN(levelMsg, fmt, ...)                                               \
    do {                                                                            \
        if (shouldLog(levelMsg))                                                    \
        {                                                                           \
            log_begin(levelMsg, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);  \
        }                                                                           \
    } while(0)

#define LOG_MSG()                                                                   \
    do {                                                                            \
        log_msg();                                                                  \
    } while(0)

#define LOG_END()                                                                        \
    do {                                                                                 \
        log_end();                                                                       \
    } while(0)

#define DLOG_(fmt, ...) LOG(LOGL_DEBUG, fmt, ##__VA_ARGS__)
#define ILOG_(fmt, ...) LOG(LOGL_INFO,  fmt, ##__VA_ARGS__)
#define WLOG_(fmt, ...) LOG(LOGL_WARN,  fmt, ##__VA_ARGS__)
#define ELOG_(fmt, ...) LOG(LOGL_ERROR, fmt, ##__VA_ARGS__)

#endif //_HLOGGER
