#ifndef BUILDER_HASHMAP_H
#define BUILDER_HASHMAP_H

#include "index_builder.h"
#include <stdint.h>

#define FNV_OFFSET_BASIS      0xcbf29ce484222325
#define FNV_PRIME             0x100000001b3
#define HASHMAP_INIT_CAPACITY 128

typedef struct {
    // Hashmap
    BuildTerm *buckets;
    size_t count;
    size_t capacity;

    // Docs
    ArrayList *docs; // ArrayList of BuildDoc

    // Metadata
    size_t total_string_bytes;
    size_t total_postings;
    size_t total_offsets;
    size_t total_doc_name_bytes;
} IndexBuilder;

IndexBuilder *hashmap_create(size_t init_capacity);
void hashmap_destroy(IndexBuilder *map);
int hashmap_insert(IndexBuilder *map, const BuildTerm *item);
int hashmap_resize(IndexBuilder *map, size_t new_capacity);
BuildTerm *hashmap_get(IndexBuilder *map, const char *key);
uint64_t hash(const char *str);

#endif
