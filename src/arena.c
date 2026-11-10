#include "arena.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

Arena *arena_create(size_t initial_capacity) {
    Arena *arena = malloc(sizeof(Arena));
    if (arena == NULL) {
        return NULL;
    }

    arena->data = malloc(initial_capacity);
    if (arena->data == NULL) {
        free(arena);
        return NULL;
    }

    arena->capacity = initial_capacity;
    arena->used = 0;

    return arena;
}

// Aligns the cursor to the nearest alignment multiple
void arena_align(Arena *arena, size_t align) {
    size_t remainder = arena->used % align;
    if (remainder != 0) {
        size_t padding = align - remainder;
        arena->used += padding; // Advance past unaligned padding
    }
}

// Appends arbitrary bytes and returns the relative byte offset
uint32_t arena_push(Arena *arena, const void *bytes, size_t size, size_t align) {
    arena_align(arena, align);

    // Grow if needed
    while (arena->used + size > arena->capacity) {
        arena->capacity *= 2;
        arena->data = realloc(arena->data, arena->capacity);
    }

    uint32_t relative_offset = (uint32_t)arena->used;
    memcpy(arena->data + relative_offset, bytes, size);
    arena->used += size;

    return relative_offset;
}

void arena_free(Arena *arena) {
    if (arena) {
        free(arena->data);
        free(arena);
    }
}
