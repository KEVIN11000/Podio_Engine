#ifndef LOGGER_H
#define LOGGER_H

typedef enum {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LogLevel;

// Initializes the logger. Opens a log file in append mode.
// Returns 0 on success, -1 on failure.
int Logger_Init(const char* log_path);

// Writes a formatted log message.
void Logger_Log(LogLevel level, const char* fmt, ...);

// Closes the log file.
void Logger_Shutdown(void);

#endif // LOGGER_H
