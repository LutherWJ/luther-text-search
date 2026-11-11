#include "flatmap.h"
#include "builder_hashmap.h"
#include "index_builder.h"
#include <stdlib.h>
#include <string.h>

FlatIndex *flatmap_create(IndexBuilder *builder) {
    FlatIndex *flatmap = malloc(sizeof(FlatIndex));
    if (flatmap == NULL) {
        return NULL;
    }

    // All of our data will be held on a single allocation for better cache locality
    size_t str_arena_size = builder->total_doc_name_bytes + builder->total_string_bytes;
    char *str_arena = malloc(str_arena_size * sizeof(char));
    if (str_arena == NULL) {
        free(flatmap);
        return NULL;
    }

    FlatPosting *posting_arena = malloc(builder->total_postings * sizeof(FlatPosting));
    if (posting_arena == NULL) {
        free(flatmap);
        free(str_arena);
        return NULL;
    }

    size_t *offset_arena = malloc(builder->total_offsets * sizeof(size_t));
    if (offset_arena == NULL) {
        free(flatmap);
        free(str_arena);
        free(posting_arena);
        return NULL;
    }

    FlatDoc *doc_arena = malloc(builder->docs->count * sizeof(FlatDoc));
    if (doc_arena == NULL) {
        free(flatmap);
        free(str_arena);
        free(posting_arena);
        free(offset_arena);
        return NULL;
    }

    FlatTerm *term_map = calloc(builder->capacity, sizeof(FlatTerm));
    if (term_map == NULL) {
        free(flatmap);
        free(str_arena);
        free(posting_arena);
        free(offset_arena);
        free(doc_arena);
        return NULL;
    }

    // cursors keeps our position so we know where to put the next value in the arena
    char *str_cursor = str_arena;
    size_t posting_cursor = 0;
    size_t offset_cursor = 0;
    // Populate the flatmap
    for (size_t i = 0; i < builder->capacity; i++) {
        BuildTerm *bucket = &builder->buckets[i];
        if (bucket->token_string != NULL) {
            size_t len = strlen(bucket->token_string);

            // Copy the term into the new hashmap
            uint64_t key_hash = hash(bucket->token_string);
            size_t index = key_hash % builder->capacity;

            while (term_map[index].hash != 0) {
                index = (index + 1) % builder->capacity;
            }

            term_map[index] = (FlatTerm){
                .hash = key_hash,
                .string_len = len,
                .string_offset = str_cursor - str_arena,
                .posting_count = bucket->postings->count,
                .posting_start = posting_cursor,
            };

            // Copy loose string into arena
            memcpy(str_cursor, bucket->token_string, len);
            str_cursor[len] = '\0'; // Terminate string
            str_cursor += len + 1;

            // Copy the postings
            for (size_t j = 0; j < bucket->postings->count; j++) {
                BuildPosting *posting = array_get(bucket->postings, j);
                posting_arena[posting_cursor++] = (FlatPosting){
                    .doc_id = posting->doc_id,
                    .offset_start = offset_cursor,
                    .offset_count = posting->offsets->count,
                };

                // Copy the offsets
                memcpy(offset_arena + offset_cursor, posting->offsets->data, posting->offsets->count * sizeof(size_t));
                offset_cursor += posting->offsets->count;
            }
        }
    }

    // Copy the documents
    for (size_t i = 0; i < builder->docs->count; i++) {
        BuildDoc *doc = array_get(builder->docs, i);
        size_t n = strlen(doc->name);
        memcpy(str_cursor, doc->name, n);
        doc_arena[i] = (FlatDoc){
            .doc_id = doc->doc_id,
            .name_offset = (size_t)(str_cursor - str_arena),
            .total_words = doc->total_words,
        };
        str_cursor[n] = '\0';
        str_cursor += n + 1;
    }

    flatmap->string_arena = str_arena;
    flatmap->string_arena_size = str_arena_size;
    flatmap->postings = posting_arena;
    flatmap->posting_count = builder->total_postings;
    flatmap->offsets = offset_arena;
    flatmap->offset_count = builder->total_offsets;
    flatmap->docs = doc_arena;
    flatmap->doc_count = builder->docs->count;
    flatmap->terms = term_map;
    flatmap->term_capacity = builder->capacity;

    return flatmap;
}

void flatmap_destroy(FlatIndex *flatmap) {
    if (flatmap == NULL) {
        return;
    }

    free(flatmap->postings);
    free(flatmap->terms);
    free(flatmap->string_arena);
    free(flatmap->docs);
    free(flatmap->offsets);
    free(flatmap);
}

FlatTerm *flatmap_get(FlatIndex *flatmap, const char *key) {
    uint64_t key_hash = hash(key);
    size_t index = key_hash % flatmap->term_capacity;

    while (flatmap->terms[index].hash != key_hash) {
        if (flatmap->terms[index].hash == 0) {
            return NULL;
        }
        index = (index + 1) % flatmap->term_capacity;
    }

    return &flatmap->terms[index];
}
