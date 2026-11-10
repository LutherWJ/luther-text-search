#ifndef BUILDER_HASHMAP_H
#define BUILDER_HASHMAP_H

#include "index_builder.h"
#include <stdint.h>

#define FNV_OFFSET_BASIS      0xcbf29ce484222325
#define FNV_PRIME             0x100000001b3
#define HASHMAP_INIT_CAPACITY 128

struct IndexBuilderMap {
    TextSearchWordEntry *buckets;
    size_t count;
    size_t capacity;
};

IndexBuilderMap *hashmap_create(size_t init_capacity);
void hashmap_destroy(IndexBuilderMap *map);
int hashmap_insert(IndexBuilderMap *map, const TextSearchWordEntry *item);
int hashmap_resize(IndexBuilderMap *map, size_t new_capacity);
TextSearchWordEntry *hashmap_get(IndexBuilderMap *map, const char *key);
uint64_t hash(const char *str);

#endif
