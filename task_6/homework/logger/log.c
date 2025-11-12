#include <stdbool.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <regex.h>
#include <stdlib.h>
#include <stdio.h>

#include "log.h"

#define COLOR_RED    "\x1b[31m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_GREEN  "\x1b[32m"
#define COLOR_CYAN   "\x1b[36m"
#define COLOR_RESET  "\x1b[0m"

#define SIZE_BUFFER 8192

struct Logger {
    LogLevel    levelLogger;
    FILE        *logFile;
    OutputMode  color_mode;
};

struct ServiceLines {
    char serv_lines[SIZE_BUFFER];
};

Logger* GetLogger(void)
{
    static Logger logger = { LOGL_DEBUG, NULL, DEFAULT_MODE };
    return &logger;
}

ServiceLines* GetServiceLines(void)
{
    static ServiceLines lines = { {0} };
    return &lines;
}

int LoggerInit(LogLevel levelLogger, const char *log_file_name, OutputMode color_mode)
{
    Logger *log = GetLogger();
    log->levelLogger = levelLogger;
    log->color_mode  = color_mode;

    if (color_mode == COLOR_MODE) {
        log->logFile = stdout;
    } else {
        log->logFile = fopen(log_file_name, "w");
        if (!log->logFile) {
            fprintf(stderr, "LoggerInit: failed to open file: %s\n", log_file_name);
            return -1;
        }
    }
    return 0;
}

bool shouldLog(LogLevel levelMsg)
{
    return (levelMsg >= GetLogger()->levelLogger);
}

const char* ColorLogMsg(LogLevel levelMsg)
{
    Logger *log = GetLogger();
    bool colored = (log->color_mode == COLOR_MODE);

    switch (levelMsg)
    {
        case LOGL_DEBUG: return colored ? COLOR_GREEN  "[DEBUG]" COLOR_RESET : "[DEBUG]";
        case LOGL_INFO:  return colored ? COLOR_CYAN   "[INFO]"  COLOR_RESET : "[INFO]";
        case LOGL_WARN:  return colored ? COLOR_YELLOW "[WARN]"  COLOR_RESET : "[WARN]";
        case LOGL_ERROR: return colored ? COLOR_RED    "[ERROR]" COLOR_RESET : "[ERROR]";
        default: return "[UNKNOWN]";
    }
}

void RemoveAnsiCodes(char *str)
{
    if (!str) return;

    regex_t regex;
    regcomp(&regex, "\x1b\\[[0-9;]*m", REG_EXTENDED);
    char *src = str, *dst = str;
    regmatch_t match;

    while (regexec(&regex, src, 1, &match, 0) == 0) {
        size_t len = (size_t)match.rm_so;
        memmove(dst, src, len);
        dst += len;
        src += match.rm_eo;
    }

    memmove(dst, src, strlen(src) + 1);
    regfree(&regex);
}

void log_begin(LogLevel levelMsg, const char *file, size_t line, const char *func, const char *fmt, ...)
{
    Logger *log = GetLogger();
    ServiceLines *srv = GetServiceLines();

    if (!log->logFile) {
        fprintf(stderr, "Logger not initialized!\n");
        return;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_info[30];
    strftime(time_info, sizeof(time_info), "%Y-%m-%d %H:%M:%S", t);

    memset(srv->serv_lines, 0, sizeof(srv->serv_lines));

    va_list args;
    va_start(args, fmt);
    vsnprintf(srv->serv_lines, sizeof(srv->serv_lines), fmt, args);
    va_end(args);

    if (log->color_mode != COLOR_MODE)
        RemoveAnsiCodes(srv->serv_lines);

    if (log->color_mode == COLOR_MODE)
        fprintf(log->logFile, COLOR_YELLOW "[%s]" COLOR_RESET "%s" COLOR_CYAN "[%s:%zu:%s]: " COLOR_RESET,
                time_info, ColorLogMsg(levelMsg), file, line, func);
    else

    fprintf(log->logFile, "[%s]%s[%s:%zu:%s]: ", time_info, ColorLogMsg(levelMsg), file, line, func);
}

void log_msg() {
     fprintf(GetLogger()->logFile, "\n%s", GetServiceLines()->serv_lines);
}

void log_end() {
    memset(GetServiceLines()->serv_lines, 0, sizeof(GetServiceLines()->serv_lines));
    fflush(GetLogger()->logFile);
}

void LoggerDeinit(void)
{
    Logger *log = GetLogger();
    if (log->logFile && log->logFile != stdout)
        fclose(log->logFile);
    log->logFile = NULL;
}
