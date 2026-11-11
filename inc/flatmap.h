#ifndef FLATMAP_H
#define FLATMAP_H

#include "index_builder.h"

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

// The single, contiguous read-only index
typedef struct {
    size_t doc_count;
    FlatDoc *docs; // Array of documents

    size_t term_capacity;
    FlatTerm *terms; // Flat hash table

    size_t posting_count;
    FlatPosting *postings; // Contiguous postings

    size_t offset_count;
    uint64_t *offsets; // Contiguous raw offsets

    size_t string_arena_size;
    char *string_arena; // All strings packed together
} FlatIndex;

FlatIndex *flatmap_create(IndexBuilder *builder);
void flatmap_destroy(FlatIndex *flatmap);
FlatTerm *flatmap_get(FlatIndex *flatmap, const char *query);

#endif
