# MINILOG

---

## PRESENTATION

Simple and easy to use stb-style logger. This is just a practice project but will definitely end up merged to [quest](https://github.com/itsjustgalileo/quest).

> **_GREAT NEWS_: THIS IS NOW THREAD-SAFE.**

---

## PREREQUISITES

- C99 compiler

---

## SUPPORTED PLATFORMS

- [X] Windows
- [X] Linux
- [X] macOS
- [ ] Web
- [ ] Mobile
  - [ ] Android
  - [ ] iOS
  
---

## HOW TO USE

1- clone the repo
```sh
git clone https://github.com/itsjustgalileo/minilog
```
2- Add the header to your project
```c
#define MINILOG_IMPLEMENTATION
#include "minilog.h"
```

---

## EXAMPLES

```c
#define MINILOG_IMPLEMENTATION
#include "../minilog.h"

static void connect_to_server(const char *host, int port)
{
    MINILOG_TRACE("Entering connect_to_server()");

    MINILOG_DEBUG("Connecting to server");

    MINILOG_INFO("Connecting to %s:%d", host, port);

    if (port != 443) {
        MINILOG_WARN("Using non-standard port: %d", port);
    }

    if (port < 1 || port > 65535) {
        MINILOG_ERROR("Invalid port number: %d", port);
        return;
    }

    MINILOG_INFO("Connection established");
}

static void process_request(int request_id, const char *username)
{
    MINILOG_TRACE("Entering process_request()");

    MINILOG_DEBUG("Processing request");

    MINILOG_INFO("Request %d received from user '%s'", request_id, username);

    if (request_id < 0) {
        MINILOG_ERROR("Request %d has an invalid ID", request_id);
    }

    if (username == NULL) {
        MINILOG_FATAL("Username is NULL");
        return;
    }

    MINILOG_TRACE("Leaving process_request()");
}

int main(void)
{
    MINILOG_TRACE("Program starting");

    MINILOG_DEBUG("Debug mode is enabled");

    MINILOG_INFO("MiniLog server connected");

    connect_to_server("example.test", 6969);

    process_request(42, "alice");

    process_request(-1, "bob");

    MINILOG_WARN("This is a warning with no additional data");

    MINILOG_ERROR("error on port %d", 6969);

    MINILOG_FATAL("fatal error");

    MINILOG_INFO("MiniLog server disconnected");

    return 0;
}
```

---

## TODO

- [ ] Add support for both log and logf
- [X] Add `set_log_function`
- [X] Make file logging only if a file is provided
- [ ] Set `set_log_format` variadics.
- [ ] Fix log file name in windows (remove `e` at the beginning)

<!--In no particular order of priority-->
- [X] Add Win32 colors
- [X] Add multithreading support
- [X] Write a `set_log_color()` function
- [ ] Add support for multiple files
- [ ] Remove C preprocessor macros from minilog

---

## REFERENCES

- [stb](https://github.com/nothings/stb)
- [tsoding's nob](https://github.com/tsoding/nob.h)
- [Variadic macros](https://en.wikipedia.org/wiki/Variadic_macro)
- [ANSI escape sequences](https://jakob-bagterp.github.io/colorist-for-python/ansi-escape-codes/standard-16-colors/#structure)
- [\_\_STDC_VERSION\_\_ values](https://sourceforge.net/p/predef/wiki/Standards/)
