#ifndef ARENA_H
#define ARENA_H

#include "shared.h"

#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>

#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT (2 * sizeof(void *))
#endif

typedef struct _Arena {
    uint8_t *buf;
    size_t size;
    size_t offset;
} Arena;

Arena arena_init(size_t size);

// TODO genericize this to allow array allocations
void *arena_alloc(Arena *arena, size_t size);

void arena_reset(Arena *arena);

void arena_deinit(Arena *arena);

#endif
