#include "shared.h"
#include <stdarg.h>

void *p_stil_malloc(size_t size, const char *file, int line) {
    if(size == 0) {
        return NULL;
    }

    void *mem = malloc(size);
    if(!mem) {
        stil_fatal("Couldn't malloc in %s at line %d", file, line);
    }

    return mem;
}

void *p_still_realloc(void *mem, size_t size, const char *file, int line) {
    if(size == 0) {
        free(mem);
        return NULL;
    }

    if(!mem) {
        return p_stil_malloc(size, file, line);
    }

    void *new_mem = realloc(mem, size);
    if(!new_mem) {
        stil_fatal("Couldn't realloc in %s at line %d", file, line);
    }

    return new_mem;
}

void stil_free(void *mem) {
    if(!mem) {
        return;
    }

    free(mem);
}

typedef struct _LogStyle {
    const char *label;
    const char *label_style;
    const char *msg_style;
} LogStyle;

static const LogStyle LOG_LEVEL_TO_STYLE[] = {
    {
        .label = "INFO",
        .label_style = ANSI_BOLD ANSI_BRIGHT_BLUE,
        .msg_style = ANSI_BLUE,
    },
    {
        .label = "WARN",
        .label_style = ANSI_BOLD ANSI_BRIGHT_YELLOW,
        .msg_style = ANSI_YELLOW,
    },
    {
        .label = "FATAL",
        .label_style = ANSI_BOLD ANSI_BRIGHT_RED,
        .msg_style = ANSI_RED,
    },
};

void p_stil_log(LogLevel level, const char *fmt, ...) {
    const LogStyle *style = &(LOG_LEVEL_TO_STYLE[level]);
    printf("%s%s%s ", style->label_style, style->label, ANSI_RESET);
    printf("%s", style->msg_style);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("%s\n", ANSI_RESET);

    if(level == LOG_FATAL) {
        exit(1);
    }
}
