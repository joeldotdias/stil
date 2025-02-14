#ifndef SHARED_H
#define SHARED_H

#include <stdio.h>
#include <stdlib.h>

/* alloc */
void *p_stil_malloc(size_t size, const char *file, int line);
#define stil_malloc(size) p_stil_malloc(size, __FILE__, __LINE__)

void *p_still_realloc(void *mem, size_t size, const char *file, int line);
#define stil_realloc(mem, size) p_stil_realloc(mem, size, __FILE__, __LINE__)

void stil_free(void *mem);

/* log */
typedef enum _LogLevel { LOG_INFO = 0, LOG_WARN, LOG_FATAL } LogLevel;
void p_stil_log(LogLevel level, const char *fmt, ...);

#define stil_info(...)  p_stil_log(LOG_INFO, __VA_ARGS__)
#define stil_warn(...)  p_stil_log(LOG_WARN, __VA_ARGS__)
#define stil_fatal(...) p_stil_log(LOG_FATAL, __VA_ARGS__)

#define ANSI_ESC(code) "\x1b[" code "m"

#define ANSI_RESET ANSI_ESC("0")
#define ANSI_BOLD  ANSI_ESC("1")

#define ANSI_BLUE          ANSI_ESC("34")
#define ANSI_BRIGHT_BLUE   ANSI_ESC("94")
#define ANSI_YELLOW        ANSI_ESC("33")
#define ANSI_BRIGHT_YELLOW ANSI_ESC("33")
#define ANSI_RED           ANSI_ESC("31")
#define ANSI_BRIGHT_RED    ANSI_ESC("91")

#endif
