#ifndef INDEX_BUILDER_H
#define INDEX_BUILDER_H

#include <array_list.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char *start;
    size_t length;
    size_t byte_offset;
} Token;

typedef struct TextSearchIndex TextSearchIndex;
typedef struct IndexBuilderMap IndexBuilderMap;
// Information about each indexed document
typedef struct {
    uint32_t doc_id;       // Sequential ID
    char *name;            // Name of the document file
    uint64_t content_hash; // Hash of document text for deduplication
    size_t total_words;    // Total word count in document
    bool is_deleted;       // Tombstone flag: true if deleted
} DocumentRecord;

// Match details for a specific document
typedef struct {
    uint32_t doc_id;
    // An "offset" is the position of the first character of the word in the document.
    // Essentially an index in a character array that starts you on the first letter of the word
    ArrayList *offsets; // Dynamic array of byte offsets (uint64_t)
} TextSearchPosting;

// The entry stored in the hash map bucket for a word
typedef struct {
    char *token_string; // Actual word being searched
    ArrayList *matches; // ArrayList of TextSearchPosting items
} TextSearchWordEntry;

/*
 * Type definitions for our flattened data model
 * This would extremely complicated to dynamically build while parsing
 * but this is serializable and provides better cache locality
 */

typedef struct {
    uint32_t doc_id;
    // Each offset is a uint64_t value
    size_t offset_count; // Number of times the word occurs in a document
    size_t offset_start; // Byte offset into the offsets arena
} Posting;

typedef struct {
    size_t string_offset; // Byte offset into string arena
    size_t string_len;
    size_t posting_offset; // Byte offset into postings arena
    size_t posting_count;
} WordEntry;

// FLattened index for better cache locality
// This gets created after tsi_finalize() is called
typedef struct {
    WordEntry *word_entries; // Array of all unique words
    size_t word_count;

    Posting *postings; // Contiguous array of all document matches
    size_t posting_count;

    uint64_t *offsets; // Contiguous array of all positional offsets
    size_t offset_count;

    char *string_arena; // Contiguous buffer of all token strings
    size_t string_arena_size;
} FlatTextIndex;

#endif
