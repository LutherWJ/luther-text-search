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

/* we will eventually migrate to this.
struct TextSearchIndex {
    bool is_finalized;
    union {
        IndexBuilderMap *buildmap; // Maps token_string -> TextSearchWordEntry
        FlatTextIndex *flatmap;    // Flattened, memory optimized immutable version of map
    } map;
    ArrayList *documents; // ArrayList of DocumentRecord
    size_t active_docs;   // Count of non-deleted documents
};
*/

struct TextSearchIndex {
    IndexBuilderMap *map;   // Maps token_string -> TextSearchWordEntry
    FlatTextIndex *flatmap; // Flattened, memory optimized immutable version of map
    ArrayList *documents;   // ArrayList of DocumentRecord
    size_t active_docs;     // Count of non-deleted documents
};

// Library API
TextSearchIndex *tsi_create();
void tsi_destroy(TextSearchIndex *index);
void tsi_index_document(TextSearchIndex *tsi, const char *name, const char *text);
bool tsi_finalize(TextSearchIndex *tsi);
bool tsi_serialize_index_to_disk(TextSearchIndex *tsi, char *file_path);
bool tsi_load_index_from_disk(TextSearchIndex *tsi, char *file_path);
TextSearchWordEntry *tsi_search_word(const TextSearchIndex *tsi, const char *word);

#endif
