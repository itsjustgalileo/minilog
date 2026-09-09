// TODO make thread-safe
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#endif /* _WIN32 */

// TODO this should not be provied by this module
#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_BUILD 1
#define BREAKPOINT __debugbreak()
#elif (defined(__GNUC__) || defined(__clang__)) && !defined(NDEBUG)
#define DEBUG_BUILD 1
#define BREAKPOINT __builtin_trap()
#else
//? exit instead of breaking in production
#define BREAKPOINT
#endif /* defined(_MSC_VER) && defined(_DEBUG) */

// TODO Improve the color system
#ifdef _WIN32
#define LOG_OUTPUT_COLOR_RED ""
#define LOG_OUTPUT_COLOR_GREEN ""
#define LOG_OUTPUT_COLOR_YELLOW ""
#define LOG_OUTPUT_COLOR_BLUE ""
#define LOG_OUTPUT_COLOR_WHITE ""
#define LOG_OUTPUT_COLOR_BLACK ""
#define LOG_OUTPUT_COLOR_RED_BG ""
#define LOG_OUTPUT_COLOR_YELLOW_BG ""
#define LOG_OUTPUT_COLOR_RESET ""
#else
// These will work with the new windows terminal
#define LOG_OUTPUT_COLOR_RED "\x1b[31m"
#define LOG_OUTPUT_COLOR_GREEN "\x1b[32m"
#define LOG_OUTPUT_COLOR_YELLOW "\x1b[33m"
#define LOG_OUTPUT_COLOR_BLUE "\x1b[34m"
#define LOG_OUTPUT_COLOR_WHITE "\x1b[37m"
#define LOG_OUTPUT_COLOR_BLACK "\x1b[30m"
#define LOG_OUTPUT_COLOR_RED_BG "\x1b[41m"
#define LOG_OUTPUT_COLOR_YELLOW_BG "\x1b[44m"
#define LOG_OUTPUT_COLOR_RESET "\x1b[0m"
#endif /* _WIN32 */

// Declaration
typedef enum LogPriority {
    LOG_PRIORITY_TRACE = 0x00,
    LOG_PRIORITY_DEBUG,
    LOG_PRIORITY_INFO,
    LOG_PRIORITY_WARN,
    LOG_PRIORITY_ERROR,
    LOG_PRIORITY_FATAL,
    LOG_PRIORITY_TODO,
} LogPriority;

#define MINILOG_TRACE(...) minilog_log_trace(__FILE__, __LINE__, __VA_ARGS__)
#define MINILOG_DEBUG(...) minilog_log_debug(__FILE__, __LINE__, __VA_ARGS__)
#define MINILOG_INFO(...) minilog_log_info(__FILE__, __LINE__, __VA_ARGS__)
#define MINILOG_WARN(...) minilog_log_warn(__FILE__, __LINE__, __VA_ARGS__)
#define MINILOG_ERROR(...) minilog_log_error(__FILE__, __LINE__, __VA_ARGS__)
#define MINILOG_FATAL(...) minilog_log_fatal(__FILE__, __LINE__, __VA_ARGS__)

#define MINILOG_TODO(...) minilog_log_todo(__FILE__, __LINE__, __VA_ARGS__)

void minilog_init(const char *path);
void minilog_shutdown(void);

void minilog_set_file(char *path);

int minilog_log_v(FILE *fp, LogPriority priority, const char *fmt, va_list ap,
                  const char *file, int line);

__attribute__((format(printf, 3, 4))) int
minilog_log_trace(const char *file, int line, const char *fmt, ...);
__attribute__((format(printf, 3, 4))) int
minilog_log_debug(const char *file, int line, const char *fmt, ...);
__attribute__((format(printf, 3, 4))) int
minilog_log_info(const char *file, int line, const char *fmt, ...);
__attribute__((format(printf, 3, 4))) int
minilog_log_warn(const char *file, int line, const char *fmt, ...);
__attribute__((format(printf, 3, 4))) int
minilog_log_error(const char *file, int line, const char *fmt, ...);
__attribute__((noreturn)) __attribute__((format(printf, 3, 4))) int
minilog_log_fatal(const char *file, int line, const char *fmt, ...);

__attribute__((format(printf, 3, 4))) int
minilog_log_error(const char *file, int line, const char *fmt, ...);

#ifdef MINILOG_IMPLEMENTATION
static char *color_from_priority(LogPriority priority)
{
    switch (priority) {
    case LOG_PRIORITY_TRACE:
        return LOG_OUTPUT_COLOR_RESET;
    case LOG_PRIORITY_DEBUG:
        return LOG_OUTPUT_COLOR_BLUE;
    case LOG_PRIORITY_INFO:
        return LOG_OUTPUT_COLOR_GREEN;
    case LOG_PRIORITY_WARN:
        return LOG_OUTPUT_COLOR_YELLOW;
    case LOG_PRIORITY_ERROR:
        return LOG_OUTPUT_COLOR_RED;
    case LOG_PRIORITY_FATAL:
        return LOG_OUTPUT_COLOR_RED_BG;
    case LOG_PRIORITY_TODO:
        return LOG_OUTPUT_COLOR_YELLOW_BG;
    default:
        return LOG_OUTPUT_COLOR_RESET;
    }
    return NULL;
}

static char *string_from_priority(LogPriority priority)
{
    switch (priority) {
    case LOG_PRIORITY_TRACE:
        return "[TRACE]   - ";
    case LOG_PRIORITY_DEBUG:
        return "[DEBUG]   - ";
    case LOG_PRIORITY_INFO:
        return "[INFO]    - ";
    case LOG_PRIORITY_WARN:
        return "[WARNING] - ";
    case LOG_PRIORITY_ERROR:
        return "[ERROR]   - ";
    case LOG_PRIORITY_FATAL:
        return "[FATAL]   - ";
    case LOG_PRIORITY_TODO:
        return "[TODO]    - ";
    default:
        return "[MINILOG] - ";
    }
    return NULL;
}

// Implementation
static FILE *log_fp;
static char *log_color;

void minilog_init(const char *path)
{
    log_fp = NULL;
    minilog_set_file(path ? path : "mini.log");
    // resetting the color
    log_color = LOG_OUTPUT_COLOR_RESET;
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_RED | FOREGROUND_GREEN
                                | FOREGROUND_BLUE);
#endif // _WIN32
    return;
}

void minilog_shutdown(void)
{
    // making sure we don't leave any colors behind
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_RED | FOREGROUND_GREEN
                                | FOREGROUND_BLUE);
#endif // _WIN32
    log_color = LOG_OUTPUT_COLOR_RESET;
    if (NULL != log_fp) {
        fclose(log_fp);
        log_fp = NULL;
    }
    return;
}

void minilog_set_file(char *path)
{
    if (log_fp)
        fclose(log_fp);
    char time_buf[256];
    time_t t = time(NULL);
    struct tm *tp = localtime(&t);
    strftime(time_buf, 256, "-%Y-%m-%d-%H:%M:%S", tp);
    char real_path[256];
    strcat(real_path, path);
    strcat(real_path, time_buf);

    log_fp = fopen(real_path, "a+");
    assert(log_fp);
    return;
}

int minilog_log_v(FILE *fp, LogPriority priority, const char *fmt, va_list ap,
                  const char *file, int line)
{
    int len = 0;

    if (NULL == fp)
        fp = log_fp;

    FILE *log_fd = stdout;
    if (priority == LOG_PRIORITY_ERROR || priority == LOG_PRIORITY_FATAL) {
        log_fd = stderr;
    }

    char *color_buf = color_from_priority(priority);
    char *priority_buf = string_from_priority(priority);

    char time_buf[256];
    time_t t = time(NULL);
    struct tm *tp = localtime(&t);
    strftime(time_buf, 256, "%Y-%b-%d - %H:%M:%S", tp);

    va_list args;
    va_copy(args, ap);
    // printing to the terminal
    // TODO make this optional only in debugging
    if (log_fd) {
        len += fprintf(log_fd, "%s[%s]%s (%s:%d) : ", color_buf, time_buf,
                       priority_buf, file, line);
        len += vfprintf(log_fd, fmt, ap);
        len += fprintf(log_fd, LOG_OUTPUT_COLOR_RESET);
        len += fprintf(log_fd, "\n");
    }
    if (fp) {
        len += fprintf(fp, "[%s]%s (%s:%d): ", time_buf, priority_buf, file,
                       line);
        len += vfprintf(fp, fmt, args);
        len += fprintf(fp, "\n");
    }
    va_end(args);

    return len;
}

int minilog_log_trace(const char *file, int line, const char *fmt, ...)
{
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_RED | FOREGROUND_GREEN
                                | FOREGROUND_BLUE);
#endif // _WIN32
    int len = 0;
    va_list ap;
    va_start(ap, fmt);
    len += minilog_log_v(log_fp, LOG_PRIORITY_TRACE, fmt, ap, file, line);
    va_end(ap);
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_RED | FOREGROUND_GREEN
                                | FOREGROUND_BLUE);
#endif // _WIN32
    return len;
}

int minilog_log_debug(const char *file, int line, const char *fmt, ...)
{
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE);
#endif // _WIN32
    int len = 0;
    va_list ap;
    va_start(ap, fmt);
    len += minilog_log_v(log_fp, LOG_PRIORITY_DEBUG, fmt, ap, file, line);
    va_end(ap);
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_RED | FOREGROUND_GREEN
                                | FOREGROUND_BLUE);
#endif // _WIN32
    return len;
}

int minilog_log_info(const char *file, int line, const char *fmt, ...)
{
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            BACKGROUD_RED | BACKGROUND_GREEN);
#endif // _WIN32
    int len = 0;
    va_list ap;
    va_start(ap, fmt);
    len += minilog_log_v(log_fp, LOG_PRIORITY_INFO, fmt, ap, file, line);
    va_end(ap);
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_RED | FOREGROUND_GREEN
                                | FOREGROUND_BLUE);
#endif // _WIN32
    return len;
}

int minilog_log_warn(const char *file, int line, const char *fmt, ...)
{
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_RED | FOREGROUND_GREEN);
#endif // _WIN32
    int len = 0;
    va_list ap;
    va_start(ap, fmt);
    len += minilog_log_v(log_fp, LOG_PRIORITY_WARN, fmt, ap, file, line);
    va_end(ap);
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_RED | FOREGROUND_GREEN
                                | FOREGROUND_BLUE);
#endif // _WIN32
    return len;
}

int minilog_log_error(const char *file, int line, const char *fmt, ...)
{
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
#endif // _WIN32

    int len = 0;
    va_list ap;
    va_start(ap, fmt);
    len += minilog_log_v(log_fp, LOG_PRIORITY_ERROR, fmt, ap, file, line);
    va_end(ap);

#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_RED | FOREGROUND_GREEN
                                | FOREGROUND_BLUE);
#endif // _WIN32
    return len;
}

int minilog_log_fatal(const char *file, int line, const char *fmt, ...)
{
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
                                | BACKGROUND_RED | FOREGROUND_INTENSITY
                                | BACKGROUND_INTENSITY);
#endif // _WIN32
    int len = 0;
    va_list ap;
    va_start(ap, fmt);
    len += minilog_log_v(log_fp, LOG_PRIORITY_FATAL, fmt, ap, file, line);
    va_end(ap);
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_RED | FOREGROUND_GREEN
                                | FOREGROUND_BLUE);
#endif // _WIN32
    if (log_fp)
        fclose(log_fp);
#ifdef DEBUG_BUILD
    (void)len;
    BREAKPOINT;
#else
    exit(len);
#endif /* DEBUG_BUILD */
}

int minilog_log_todo(const char *file, int line, const char *fmt, ...)
{
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            BACKGROUD_RED | BACKGROUND_GREEN);
#endif // _WIN32
    int len = 0;
    va_list ap;
    va_start(ap, fmt);
    len += minilog_log_v(log_fp, LOG_PRIORITY_TODO, fmt, ap, file, line);
    va_end(ap);
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_RED | FOREGROUND_GREEN
                                | FOREGROUND_BLUE);
#endif // _WIN32
    return len;
}

#endif /* MINILOG_IMPLEMENTATION */
