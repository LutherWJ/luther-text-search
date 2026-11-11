#include "builder_hashmap.h"
#include "array_list.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint64_t hash(const char *str) {
    uint64_t hash = FNV_OFFSET_BASIS;
    while (*str != '\0') {
        hash ^= (uint8_t)*str++;
        hash *= FNV_PRIME;
    }
    return hash;
}

IndexBuilder *hashmap_create(size_t init_capacity) {
    IndexBuilder *map = malloc(sizeof(IndexBuilder));
    if (map == NULL) {
        return NULL;
    }

    map->count = 0;
    map->capacity = init_capacity;
    map->buckets = calloc(init_capacity, sizeof(BuildTerm));
    if (map->buckets == NULL) {
        free(map);
        return NULL;
    }

    map->docs = array_create(sizeof(BuildDoc), ARRAYLIST_INIT_CAPACITY);
    if (map->docs == NULL) {
        free(map);
        free(map->buckets);
        return NULL;
    }

    map->total_postings = 0;
    map->total_offsets = 0;
    map->total_string_bytes = 0;
    map->total_doc_name_bytes = 0;

    return map;
}

int hashmap_resize(IndexBuilder *map, size_t new_capacity) {
    if (map == NULL || new_capacity == 0 || new_capacity <= map->count) {
        return 1;
    }

    BuildTerm *new_buckets = calloc(new_capacity, sizeof(BuildTerm));
    if (new_buckets == NULL) {
        return 1;
    }

    BuildTerm *old_buckets = map->buckets;
    size_t old_capacity = map->capacity;

    map->buckets = new_buckets;
    map->capacity = new_capacity;

    for (size_t i = 0; i < old_capacity; i++) {
        if (old_buckets[i].token_string != NULL) {
            uint64_t index = hash(old_buckets[i].token_string) % new_capacity;
            while (map->buckets[index].token_string != NULL) {
                index = (index + 1) % new_capacity;
            }
            map->buckets[index] = old_buckets[i];
        }
    }

    free(old_buckets);
    return 0;
}

int hashmap_insert(IndexBuilder *map, const BuildTerm *item) {
    // Resize the array if >=70% of slots are filled
    if (map->count >= ((map->capacity * 7) / 10)) {
        if (hashmap_resize(map, map->capacity * 2) != 0) {
            return 1;
        }
    }

    uint64_t index = hash(item->token_string) % map->capacity;
    BuildTerm *bucket = &map->buckets[index];

    // Look for a bucket to fill or merge
    // loop is infinite but there will always be a bucket slot to fill
    for (;;) {
        // Fill bucket
        if (bucket->token_string == NULL) {
            memcpy(bucket, item, sizeof(BuildTerm));
            map->count++;
            return 0;
        }

        // Merge bucket
        // pray there were no duplicates because I am NOT checking
        if (strcmp(bucket->token_string, item->token_string) == 0) {
            if (item->postings != NULL) {
                for (size_t i = 0; i < item->postings->count; i++) {
                    BuildPosting *new_item = array_get(item->postings, i);
                    if (array_push(bucket->postings, new_item)) {
                        return 1;
                    }
                }
            }
            return 0;
        }

        index = (index + 1) % (map->capacity); // Onto the next bucket
        bucket = &map->buckets[index];
    }
    return 1; // unreachable, something has gone terribly wrong
}

BuildTerm *hashmap_get(IndexBuilder *map, const char *key) {
    if (map->count == 0) {
        return NULL;
    }

    uint64_t index = hash(key) % map->capacity;
    BuildTerm *bucket = &map->buckets[index];

    for (size_t i = 0; i < map->capacity; i++) {

        if (bucket->token_string == NULL) {
            return NULL;
        }

        // MAKE SURE THE KEY MATCHES BECAUSE INDEX COULD BE WRONG!!!
        if (strcmp(bucket->token_string, key) == 0) {
            return bucket;
        }

        index = (index + 1) % (map->capacity); // Onto the next bucket
        bucket = &map->buckets[index];
    }
    return NULL;
}

void hashmap_destroy(IndexBuilder *map) {
    if (map == NULL) {
        return;
    }
    for (size_t i = 0; i < map->capacity; i++) {
        BuildTerm *bucket = &map->buckets[i];
        if (bucket->token_string != NULL) {
            free(bucket->token_string);
        }
        if (bucket->postings != NULL) {
            for (size_t j = 0; j < bucket->postings->count; j++) {
                BuildPosting *posting = array_get(bucket->postings, j);
                if (posting != NULL && posting->offsets != NULL) {
                    array_free(posting->offsets);
                }
            }
            array_free(bucket->postings);
        }
    }

    for (size_t i = 0; i < map->docs->count; i++) {
        BuildDoc *doc = array_get(map->docs, i);
        free(doc->name);
    }
    array_free(map->docs);
    free(map->buckets);
    free(map);
}
