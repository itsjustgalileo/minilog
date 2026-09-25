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
// TODO the debug build check should be more robust
#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_BUILD 1
extern void __debugbreak();
#define BREAKPOINT __debugbreak()
#elif (defined(__GNUC__) || defined(__clang__)) && !defined(NDEBUG)
#define DEBUG_BUILD 1
#define BREAKPOINT __builtin_trap()
#else
//? exit instead of breaking in production
#define BREAKPOINT
#endif /* defined(_MSC_VER) && defined(_DEBUG) */

#if (defined(__cplusplus) && (__cplusplus >= 201702L))                         \
    || (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202311L))
#define NORETURN [[noreturn]]
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define NORETURN _Noreturn
#elif defined(_MSC_VER)
#define NORETURN __declspec(noreturn)
#elif defined(__GNUC__) || defined(__clang__)
#define NORETURN __attribute__((noreturn))
#else
#define NORETURN
#endif /* (defined(__cplusplus) && (__cplusplus >= 201103L)) ||                \
          (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202311L)) */

#if (defined(__GNUC__) || defined(__clang__))
#define PRINTF_FORMAT(a, b) __attribute__((format(printf, a, b)))
#elif (defined(__GNUC__) && defined(_WIN32)) || defined(__MINGW32__)
#define PRINTF_FORMAT(a, b) __attribute__((format(__MINGW_PRINTF_FORMAT, a, b)))
#else
#define PRINTF_FORMAT
#endif /* defined(__GNUC__) || defined(__clang__) */

#ifdef _MSC_VER
#define MINICALL __stdcall
#else
#define MINICALL
#endif /* _MSC_VER */

// Minilog macros

#ifdef _WIN32
#define LOG_COLOR_RED ""
#define LOG_COLOR_GREEN ""
#define LOG_COLOR_YELLOW ""
#define LOG_COLOR_BLUE ""
#define LOG_COLOR_WHITE ""
#define LOG_COLOR_BLACK ""
#define LOG_COLOR_RED_BG ""
#define LOG_COLOR_BLUE_BG ""
#define LOG_COLOR_YELLOW_BG ""
#define LOG_COLOR_RESET ""
#else
// These will also work with the new windows terminal
#define LOG_COLOR_RED "\x1b[31m"
#define LOG_COLOR_GREEN "\x1b[32m"
#define LOG_COLOR_YELLOW "\x1b[33m"
#define LOG_COLOR_BLUE "\x1b[34m"
#define LOG_COLOR_WHITE "\x1b[37m"
#define LOG_COLOR_BLACK "\x1b[30m"
#define LOG_COLOR_RED_BG "\x1b[41m"
#define LOG_COLOR_BLUE_BG "\x1b[46m"
#define LOG_COLOR_RESET "\x1b[0m"
#endif /* _WIN32 */

#define MINILOG_TRACE(...) minilog_log_trace(__FILE__, __LINE__, __VA_ARGS__)
#define MINILOG_DEBUG(...) minilog_log_debug(__FILE__, __LINE__, __VA_ARGS__)
#define MINILOG_INFO(...) minilog_log_info(__FILE__, __LINE__, __VA_ARGS__)
#define MINILOG_WARN(...) minilog_log_warn(__FILE__, __LINE__, __VA_ARGS__)
#define MINILOG_ERROR(...) minilog_log_error(__FILE__, __LINE__, __VA_ARGS__)
#define MINILOG_FATAL(...) minilog_log_fatal(__FILE__, __LINE__, __VA_ARGS__)
#define MINILOG_TODO(...) minilog_log_todo(__FILE__, __LINE__, __VA_ARGS__)

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

    // Setup
    typedef enum LogPriority {
        LOG_PRIORITY_TRACE = 0x00,
        LOG_PRIORITY_DEBUG,
        LOG_PRIORITY_INFO,
        LOG_PRIORITY_WARN,
        LOG_PRIORITY_ERROR,
        LOG_PRIORITY_FATAL,
        LOG_PRIORITY_TODO,

        // For looping
        LOG_PRIORITY_SENTINELLE = 0x7fff
    } LogPriority;

    typedef enum LogOutput {
        LOG_OUTPUT_DEFAULT = 0x00, // prints to file and stdout
        LOG_OUTPUT_STDOUT,
        LOG_OUTPUT_STDERR,
        LOG_OUTPUT_FILE,

        // For looping
        LOG_OUTPUT_SENTINELLE = 0x7fff,
    } LogOutput;

    typedef int (*MINICALL log_fn_ptr)(const char *file, int line,
                                       LogPriority priority, LogOutput output,
                                       const char *fmt, va_list ap);

    extern void MINICALL minilog_init(const char *path, log_fn_ptr fn);
    extern void MINICALL minilog_shutdown(void);

    extern void MINICALL minilog_set_log_color(int target, int color);
    extern void MINICALL minilog_set_log_file(const char *path);
    extern void MINICALL minilog_set_log_function(log_fn_ptr log_fn);
    extern void MINICALL minilog_set_log_output(const LogOutput *output);
    extern void MINICALL minilog_set_log_format(char *format);

    // Printing functions
    extern PRINTF_FORMAT(3, 4) int MINICALL
        minilog_log_trace(const char *file, int line, const char *fmt, ...);

    extern PRINTF_FORMAT(3, 4) int MINICALL
        minilog_log_debug(const char *file, int line, const char *fmt, ...);

    extern PRINTF_FORMAT(3, 4) int MINICALL
        minilog_log_info(const char *file, int line, const char *fmt, ...);

    extern PRINTF_FORMAT(3, 4) int MINICALL
        minilog_log_warn(const char *file, int line, const char *fmt, ...);

    extern PRINTF_FORMAT(3, 4) int MINICALL
        minilog_log_error(const char *file, int line, const char *fmt, ...);

    extern NORETURN PRINTF_FORMAT(3, 4) int MINICALL
        minilog_log_fatal(const char *file, int line, const char *fmt, ...);

    extern PRINTF_FORMAT(3, 4) int MINICALL
        minilog_log_todo(const char *file, int line, const char *fmt, ...);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

//////////////////////////
// Implementation       //
//////////////////////////
#ifdef MINILOG_IMPLEMENTATION

#define MUTEX_IMPLEMENTATION
#include "mutex/mutex.h"

static FILE *log_fp;
static FILE *log_fd;

static LogOutput log_output;
static LogPriority log_priotity;
static log_fn_ptr log_fn;

static char time_buf[256];
static char *log_color;
static char *log_format;

static Mutex *log_lock;
static Mutex *log_fn_lock;

// Helpers
static char *color_from_priority(LogPriority priority)
{
    lock_mutex(log_lock);
    switch (priority) {
    case LOG_PRIORITY_TRACE:
        return LOG_COLOR_RESET;
    case LOG_PRIORITY_DEBUG:
        return LOG_COLOR_BLUE;
    case LOG_PRIORITY_INFO:
        return LOG_COLOR_GREEN;
    case LOG_PRIORITY_WARN:
        return LOG_COLOR_YELLOW;
    case LOG_PRIORITY_ERROR:
        return LOG_COLOR_RED;
    case LOG_PRIORITY_FATAL:
        return LOG_COLOR_RED_BG;
    case LOG_PRIORITY_TODO:
        return LOG_COLOR_BLUE_BG;
    default:
        return LOG_COLOR_RESET;
    }
    return NULL;
    unlock_mutex(log_lock);
}

static char *string_from_priority(LogPriority priority)
{
    lock_mutex(log_lock);
    switch (priority) {
    case LOG_PRIORITY_TRACE:
        return "TRACE";
    case LOG_PRIORITY_DEBUG:
        return "DEBUG";
    case LOG_PRIORITY_INFO:
        return "INFO";
    case LOG_PRIORITY_WARN:
        return "WARNING";
    case LOG_PRIORITY_ERROR:
        return "ERROR";
    case LOG_PRIORITY_FATAL:
        return "FATAL";
    case LOG_PRIORITY_TODO:
        return "TODO";
    default:
        return "MINILOG";
    }
    unlock_mutex(log_lock);
    return NULL;
}

static void set_console_text_color(LogPriority priority)
{
    lock_mutex(log_lock);
    switch (priority) {
    case LOG_PRIORITY_TRACE: {
#ifdef _WIN32
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                                FOREGROUND_RED | FOREGROUND_GREEN
                                    | FOREGROUND_BLUE);
#else
        log_color = LOG_COLOR_RESET;
#endif /* _WIN32 */
    } break;

    case LOG_PRIORITY_DEBUG: {
#ifdef _WIN32
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                                FOREGROUND_BLUE);
#else
        log_color = LOG_COLOR_BLUE;
#endif // _WIN32
    } break;

    case LOG_PRIORITY_INFO: {
#ifdef _WIN32
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                                FOREGROUND_GREEN);
#else
        log_color = LOG_COLOR_GREEN;
#endif // _WIN32
    } break;

    case LOG_PRIORITY_WARN: {
#ifdef _WIN32
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                                FOREGROUND_RED | FOREGROUND_GREEN);
#else
        log_color = LOG_COLOR_YELLOW;
#endif // _WIN32

    } break;

    case LOG_PRIORITY_ERROR: {
#ifdef _WIN32
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                                FOREGROUND_RED);
#else
        log_color = LOG_COLOR_RED;
#endif // _WIN32
    } break;

    case LOG_PRIORITY_FATAL: {
#ifdef _WIN32
        SetConsoleTextAttribute(
            GetStdHandle(STD_OUTPUT_HANDLE),
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_RED
                | FOREGROUND_INTENSITY | BACKGROUND_INTENSITY);
#else
        log_color = LOG_COLOR_RED_BG;
#endif // _WIN32
    } break;
    case LOG_PRIORITY_TODO: {
#ifdef _WIN32
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                                FOREGROUND_INTENSITY | BACKGROUND_BLUE
                                    | BACKGROUND_GREEN);
#else
        log_color = LOG_COLOR_BLUE_BG;
#endif // _WIN32
    } break;
    default: {
#ifdef _WIN32
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                                FOREGROUND_RED | FOREGROUND_GREEN
                                    | FOREGROUND_BLUE);
#else
        log_color = LOG_COLOR_RESET;
#endif /* _WIN32 */
    } break;
    }

#ifdef __unix__
    printf("%s", log_color);
#endif /* __unix__ */

    unlock_mutex(log_lock);
    return;
}

static char *get_time_point(void)
{
    lock_mutex(log_lock);
    time_t t = time(NULL);
    struct tm *tp = localtime(&t);
#ifdef _WIN32
    strftime(time_buf, 256, "%Y-%m-%d-%H-%M-%S", tp);
#else
    strftime(time_buf, 256, "%Y-%m-%d-%H:%M:%S", tp);
#endif // _WIN32

    unlock_mutex(log_lock);
    return time_buf;
}

// Default print function if none is provided
static inline int minilog_log_v(const char *file, int line,
                                LogPriority priority, LogOutput output,
                                const char *fmt, va_list ap)
{
    lock_mutex(log_lock);

    int len = 0;

    minilog_set_log_output(&output);

    switch (log_output) {
    case LOG_OUTPUT_DEFAULT:
        log_fd = stdout;
        break;
    case LOG_OUTPUT_STDOUT:
        log_fd = stdout;
        break;
    case LOG_OUTPUT_STDERR:
        log_fd = stderr;
        break;
    case LOG_OUTPUT_FILE:
        log_fd = NULL;
        break;
    default:
        break;
    }

    if (log_fd
        && (priority == LOG_PRIORITY_ERROR || priority == LOG_PRIORITY_FATAL)) {
        log_fd = stderr;
    }

    char *log_color = color_from_priority(priority);
    char *priority_buf = string_from_priority(priority);
    char *time_buf = get_time_point();

    if (log_fd == stdout || log_fd == stderr) {
        set_console_text_color(priority);
        va_list args;
        va_copy(args, ap);

        len += fprintf(log_fd, "%s%s:%d - [%s][%s]%s: ", log_color, file, line,
                       time_buf, priority_buf, LOG_COLOR_RESET);
        len += vfprintf(log_fd, fmt, args);
        len += fprintf(log_fd, "\n");

        va_end(args);
        set_console_text_color(LOG_PRIORITY_TRACE);
    }

    if (log_fp) {
        va_list args;
        va_copy(args, ap);

        len += fprintf(log_fp, "%s:%d - [%s][%s]: ", file, line, time_buf,
                       priority_buf);
        len += vfprintf(log_fp, fmt, args);
        len += fprintf(log_fp, "\n");

        va_end(args);
    }

    fflush(log_fd);
    fflush(log_fp);

    unlock_mutex(log_lock);

    return len;
}

// Definitions
void minilog_init(const char *path, log_fn_ptr fn)
{
    log_lock = create_mutex();

    minilog_set_log_file(path);
    minilog_set_log_function(fn);
    minilog_set_log_format(log_format);
    // resetting the colornnn
    set_console_text_color(LOG_PRIORITY_TRACE);
    return;
}

void minilog_shutdown(void)
{
    lock_mutex(log_lock);
    // making sure we don't leave any colors behind
    set_console_text_color(LOG_PRIORITY_TRACE);
    if (NULL != log_fp) {
        fclose(log_fp);
        log_fp = NULL;
    }

    destroy_mutex(log_lock);
    return;
}

void minilog_set_log_function(log_fn_ptr fn)
{
    lock_mutex(log_fn_lock);
    if (fn) {
        log_fn = fn;
    } else {
        log_fn = &minilog_log_v;
    }
    unlock_mutex(log_fn_lock);
    return;
}

// TODO write implementation
void minilog_set_log_format(char *format)
{
    lock_mutex(log_lock);
    // TODO handle variadics
    if (format) {
        for (char *p = format; *p++;) {
            switch (*p) {
            case 'P':
                // Priority

                break;
            case 'T':
                // Time
                break;
            case 'f':
                // file
                break;
            case 'l':
                // line
                break;
            }
        }
        log_format = format;
    } else {
        log_format = "%f:%l - [%T][%P]: ";
    }
    unlock_mutex(log_lock);
    return;
}

void minilog_set_log_file(const char *path)
{
    lock_mutex(log_lock);
    if (NULL == path) {
        unlock_mutex(log_lock);
        return;
    }

    if (NULL != log_fp) {
        fclose(log_fp);
    }

    char *time_buf = get_time_point();
    char real_path[256] = "\0";
    strcat(real_path, path);
    strcat(real_path, time_buf);
    printf("%s\n", real_path);
    log_fp = fopen(real_path, "a+");
    assert(log_fp);
    unlock_mutex(log_lock);
    return;
}

void minilog_set_log_output(const LogOutput *output)
{
    lock_mutex(log_lock);
    log_output = *output;
    unlock_mutex(log_lock);
    return;
}

int minilog_log_trace(const char *file, int line, const char *fmt, ...)
{
    int len = 0;
    va_list ap;
    va_start(ap, fmt);
    len += log_fn(file, line, LOG_PRIORITY_TRACE, log_output, fmt, ap);
    va_end(ap);
    return len;
}

int minilog_log_debug(const char *file, int line, const char *fmt, ...)
{
    int len = 0;
    va_list ap;
    va_start(ap, fmt);
    len += log_fn(file, line, LOG_PRIORITY_DEBUG, log_output, fmt, ap);
    va_end(ap);
    return len;
}

int minilog_log_info(const char *file, int line, const char *fmt, ...)
{
    int len = 0;
    va_list ap;
    va_start(ap, fmt);
    len += log_fn(file, line, LOG_PRIORITY_INFO, log_output, fmt, ap);
    va_end(ap);
    return len;
}

int minilog_log_warn(const char *file, int line, const char *fmt, ...)
{
    int len = 0;
    va_list ap;
    va_start(ap, fmt);
    len += log_fn(file, line, LOG_PRIORITY_WARN, log_output, fmt, ap);
    va_end(ap);
    return len;
}

int minilog_log_error(const char *file, int line, const char *fmt, ...)
{
    int len = 0;
    va_list ap;
    va_start(ap, fmt);
    len += log_fn(file, line, LOG_PRIORITY_ERROR, log_output, fmt, ap);
    va_end(ap);
    return len;
}

int minilog_log_fatal(const char *file, int line, const char *fmt, ...)
{
    int len = 0;
    va_list ap;
    va_start(ap, fmt);
    len += log_fn(file, line, LOG_PRIORITY_FATAL, log_output, fmt, ap);
    va_end(ap);
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
    int len = 0;
    va_list ap;
    va_start(ap, fmt);
    len += log_fn(file, line, LOG_PRIORITY_TODO, log_output, fmt, ap);
    va_end(ap);
    return len;
}

#endif /* MINILOG_IMPLEMENTATION */
