/*
 * This file serves as the API for the library.
 * Internal types and logic will all be hidden away in other files
 */

#ifndef TEXT_SEARCH_H
#define TEXT_SEARCH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct TextSearchIndex TextSearchIndex;

typedef enum {
    TSI_SUCCESS = 0,
    TSI_ERROR_NOT_FINALIZED,
    TSI_ERROR_ALREADY_FINALIZED,
    TSI_ERROR_CORRUPT_HEADER,
    TSI_ERROR_IO,
    TSI_ERROR_OOM,
} TsiStatus;

// All occurances of a word within a document
typedef struct {
    char *doc_name;
    uint64_t *offsets; // All position offsets containing the word within the document
    size_t offset_count;
    uint32_t doc_id;
} TextSearchMatch;

// All occurences of a word within an index
typedef struct {
    const char *query;        // Searched word
    TextSearchMatch *matches; // Occurences of a word within a single doc
    size_t match_count;       // Number of docs with occurences
} TextSearchResult;

// Library API
TextSearchIndex *tsi_create();
void tsi_destroy(TextSearchIndex *index);
void tsi_index_document(TextSearchIndex *tsi, const char *name, const char *text);
TsiStatus tsi_finalize(TextSearchIndex *tsi);
TsiStatus tsi_serialize_to_disk(TextSearchIndex *tsi, char *file_path);
TsiStatus tsi_load_from_disk(TextSearchIndex *tsi, char *file_path);
TextSearchResult *tsi_search_word(const TextSearchIndex *tsi, const char *word);
void tsi_free_result(TextSearchResult *result);

#endif
