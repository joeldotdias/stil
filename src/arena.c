#include "arena.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

static inline bool is_power_of_two(uintptr_t x) { return (x & (x - 1)) == 0; }

static uintptr_t align_forward(uintptr_t ptr, size_t align) {
    assert(is_power_of_two(align));

    uintptr_t align_mask = (uintptr_t)align;

    // offset of ptr from nearest aligned address
    uintptr_t offset = ptr & (align_mask - 1);

    if(offset != 0) {
        ptr += align_mask - offset;
    }
    return ptr;
}

Arena arena_init(size_t size) {
    void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(mem == MAP_FAILED) {
        stil_fatal("Couldn't mmap memory for arena: %s", strerror(errno));
    }
    return (Arena){.buf = (uint8_t *)mem, .size = size, .offset = 0};
}

static void *arena_alloc_align(Arena *arena, size_t size, size_t align) {
    uintptr_t curr_ptr = (uintptr_t)arena->buf + (uintptr_t)arena->offset;
    uintptr_t offset = align_forward(curr_ptr, align);
    offset -= (uintptr_t)arena->buf;

    if(offset + size > arena->size) {
        stil_fatal("Out of memory in the arena");
    }

    void *ptr = &arena->buf[offset];
    arena->offset = offset + size;
    memset(ptr, 0, size);
    return ptr;
}

void *arena_alloc(Arena *arena, size_t size) {
    return arena_alloc_align(arena, size, DEFAULT_ALIGNMENT);
}

void arena_reset(Arena *arena) { arena->offset = 0; }

void arena_deinit(Arena *arena) { munmap(arena->buf, arena->size); }
