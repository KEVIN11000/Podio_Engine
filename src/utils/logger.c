#include "logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static FILE* g_logFile = NULL;

static const char* level_str(LogLevel level) {
    switch (level) {
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARN";
        case LOG_ERROR: return "ERROR";
        default:        return "????";
    }
}

int Logger_Init(const char* log_path) {
    g_logFile = fopen(log_path, "a");
    if (!g_logFile) {
        return -1;
    }
    return 0;
}

void Logger_Log(LogLevel level, const char* fmt, ...) {
    if (!g_logFile) return;

    // Timestamp
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    fprintf(g_logFile, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] ",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec,
            level_str(level));

    // Message
    va_list args;
    va_start(args, fmt);
    vfprintf(g_logFile, fmt, args);
    va_end(args);

    fprintf(g_logFile, "\n");
    fflush(g_logFile);
}

void Logger_Shutdown(void) {
    if (g_logFile) {
        fclose(g_logFile);
        g_logFile = NULL;
    }
}
