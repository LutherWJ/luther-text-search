#include "flatmap.h"
#include "builder_hashmap.h"
#include "index_builder.h"
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

FlatIndex *flatmap_create(IndexBuilder *builder) {
    FlatIndex *flatmap = malloc(sizeof(FlatIndex));
    if (flatmap == NULL) {
        return NULL;
    }

    // All of our data will be held on a single allocation for better cache locality
    // We start by calculating our sizes
    alignas(8) size_t str_size = (builder->total_doc_name_bytes + builder->total_string_bytes) * sizeof(char);
    alignas(8) size_t doc_size = builder->docs->count * sizeof(FlatDoc);
    // We use capacity here to keep the term array as a hashmap with 70% load factor.
    alignas(8) size_t term_size = builder->capacity * sizeof(FlatTerm);
    alignas(8) size_t posting_size = builder->total_postings * sizeof(FlatPosting);
    alignas(8) size_t offset_size = builder->total_offsets * sizeof(size_t);

    flatmap->blob_size = str_size + doc_size + term_size + posting_size + offset_size;
    flatmap->blob = malloc(flatmap->blob_size);
    if (flatmap->blob == NULL) {
        free(flatmap);
        return NULL;
    }

    // Set the view pointers
    char *base = (char *)flatmap->blob;
    flatmap->docs = (FlatDoc *)base;
    flatmap->terms = (FlatTerm *)(base + doc_size);
    flatmap->postings = (FlatPosting *)(base + doc_size + term_size);
    flatmap->offsets = (size_t *)(base + doc_size + term_size + posting_size);
    flatmap->strings = (char *)(base + doc_size + term_size + posting_size + offset_size);

    // Set the counts
    flatmap->strings_size = str_size;
    flatmap->doc_count = builder->docs->count;
    flatmap->term_capacity = builder->capacity;
    flatmap->posting_count = builder->total_postings;
    flatmap->offset_count = builder->total_offsets;

    // Set the metadata!
    flatmap->is_mmap = false;

    // cursors keeps our position so we know where to put the next value in the arena
    char *str_cursor = flatmap->strings;
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

            while (flatmap->terms[index].hash != 0) {
                index = (index + 1) % builder->capacity;
            }

            flatmap->terms[index] = (FlatTerm){
                .hash = key_hash,
                .string_len = len,
                .string_offset = str_cursor - flatmap->strings,
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
                flatmap->postings[posting_cursor++] = (FlatPosting){
                    .doc_id = posting->doc_id,
                    .offset_start = offset_cursor,
                    .offset_count = posting->offsets->count,
                };

                // Copy the offsets
                memcpy(flatmap->offsets + offset_cursor, posting->offsets->data,
                       posting->offsets->count * sizeof(size_t));
                offset_cursor += posting->offsets->count;
            }
        }
    }

    // Copy the documents
    for (size_t i = 0; i < builder->docs->count; i++) {
        BuildDoc *doc = array_get(builder->docs, i);
        size_t n = strlen(doc->name);
        memcpy(str_cursor, doc->name, n);
        flatmap->docs[i] = (FlatDoc){
            .doc_id = doc->doc_id,
            .name_offset = (size_t)(str_cursor - flatmap->strings),
            .total_words = doc->total_words,
        };
        str_cursor[n] = '\0';
        str_cursor += n + 1;
    }

    return flatmap;
}

void flatmap_destroy(FlatIndex *flatmap) {
    if (flatmap != NULL) {
        free(flatmap->blob);
        free(flatmap);
    }
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
