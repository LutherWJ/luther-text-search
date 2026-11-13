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

// Information about each indexed document
typedef struct {
    uint32_t doc_id;    // Sequential ID
    char *name;         // Name of the document file
    size_t total_words; // Total word count in document
} BuildDoc;

// Match details for a specific document
typedef struct {
    uint32_t doc_id;
    // An "offset" is the position of the first character of the word in the document.
    // Essentially an index in a character array that starts you on the first letter of the word
    ArrayList *offsets; // Dynamic array of byte offsets (uint64_t)
} BuildPosting;

// The entry stored in the hash map bucket for a word
typedef struct {
    char *token_string; // Actual word being searched
    uint64_t hash;
    ArrayList *postings; // ArrayList of TextSearchPosting items
} BuildTerm;

#endif
