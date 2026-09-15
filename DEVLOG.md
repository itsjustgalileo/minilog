# DEVLOG

---

## [15-Sep-2026] - Cleanup and refactoring - 0.0.2

+ Support for `MinGw32`'s `printf` format attribute.
+ `printf` format attribute macro.
+ `noreturn` attribute macro.
+ Support for `set_log_function()` function through `log_fn_ptr` type.
+ Support for `set-log_file()` function.

---

## [09-Sep-2026] - TODO and Win32 - 0.0.1

+ Support for `MINILOG_TODO` macro.
* Fixed `return` before resetting colors on Windows.
* Fixed variadic support in `minilog_log_v`.
* Improved file handling.

---

## [08-Sep-2026] - Initial commit - 0.0.0

+ Initial version with UNIX-like and Windows support.
