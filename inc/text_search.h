/*
 * This file serves as the API for the library.
 * Internal types and helpers will all be hidden away in other files
 */

#ifndef TEXT_SEARCH_H
#define TEXT_SEARCH_H

#include "array_list.h"
#include "index_builder.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct TextSearchIndex {
    IndexBuilderMap *map; // Maps token_string -> TextSearchWordEntry
    ArrayList *documents; // ArrayList of DocumentRecord
    size_t active_docs;   // Count of non-deleted documents
};

// Library API
TextSearchIndex *tsi_create();
void tsi_destroy(TextSearchIndex *index);
void tsi_index_document(TextSearchIndex *tsi, const char *name, const char *text);
TextSearchWordEntry *tsi_search_word(TextSearchIndex *tsi, const char *word);

#endif
