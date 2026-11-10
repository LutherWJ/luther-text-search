#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>
#include <stdlib.h>

typedef struct {
    uint8_t *data;
    size_t capacity;
    size_t used;
} Arena;

Arena *arena_create(size_t initial_capacity);
void arena_align(Arena *arena, size_t align);
uint32_t arena_push(Arena *arena, const void *bytes, size_t size, size_t align);
void arena_free(Arena *arena);

#endif
