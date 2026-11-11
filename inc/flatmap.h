#ifndef FLATMAP_H
#define FLATMAP_H

#include "index_builder.h"
#define FOURCC(a, b, c, d)                                                                                             \
    ((uint32_t)(uint8_t)(a) | ((uint32_t)(uint8_t)(b) << 8) | ((uint32_t)(uint8_t)(c) << 16) |                         \
     ((uint32_t)(uint8_t)(d) << 24))

#define CTEXTSEARCH_MAGIC_NUMBER   FOURCC('S', 'L', 'V', 'T')
#define CTEXTSEARCH_VERSION_NUMBER 1

/*
 * Type definitions for our flattened data model
 * This would extremely complicated to dynamically build while parsing
 * but this is serializable and provides better cache locality
 */

// Document metadata referencing string arena
typedef struct {
    uint32_t doc_id;
    uint32_t name_offset; // Byte offset into string_arena
    uint64_t content_hash;
    size_t total_words;
} FlatDoc;

// Match details referencing offsets arena
typedef struct {
    uint32_t doc_id;
    uint32_t offset_start; // Index into offsets array
    uint32_t offset_count; // Number of occurrences
} FlatPosting;

// Term bucket referencing string & postings arenas
typedef struct {
    uint64_t hash;          // Cached hash (0 if empty bucket)
    uint32_t string_offset; // Byte offset into string_arena
    uint32_t string_len;
    uint32_t posting_start; // Index into postings array
    uint32_t posting_count; // Number of documents containing this term
} FlatTerm;

typedef struct {
    uint32_t magic;
    uint32_t version;

    // Counts
    uint32_t doc_count;
    uint32_t term_capacity;
    uint32_t posting_count;
    uint32_t offset_count;
    uint32_t string_arena_size;

    // Byte offsets from the start of the buffer/file
    uint32_t docs_offset;
    uint32_t terms_offset;
    uint32_t postings_offset;
    uint32_t offsets_offset;
    uint32_t strings_offset;

    uint32_t total_file_size;
} IndexHeader;

// The single, contiguous read-only index
typedef struct {
    // View pointers
    FlatDoc *docs;         // Array of documents
    FlatTerm *terms;       // Flat hash table
    FlatPosting *postings; // Contiguous postings
    uint64_t *offsets;     // Contiguous raw offsets
    char *strings;         // All strings packed together

    // Counts
    size_t doc_count;     // Count of documents indexed
    size_t term_capacity; // Max capacity (NOT COUNT) of the term hashmap
    size_t posting_count; // Total amount of postings
    size_t offset_count;  // Total amount of offsets
    size_t strings_size;  // Total length of the strings array

    // Single allocation where the data actually lives
    void *blob;       // Base pointer to the arena
    size_t blob_size; // Total allocation size
    bool is_mmap;     // true if loaded via mmap, false if malloc
} FlatIndex;

FlatIndex *flatmap_create(IndexBuilder *builder);
void flatmap_destroy(FlatIndex *flatmap);
FlatTerm *flatmap_get(FlatIndex *flatmap, const char *query);

#endif
