#include "text_search.h"
#include "array_list.h"
#include "builder_hashmap.h"
#include "index_builder.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

TextSearchIndex *tsi_create() {
    TextSearchIndex *tsi = malloc(sizeof(TextSearchIndex));
    if (tsi == NULL) {
        return NULL;
    }

    ArrayList *document_list = array_create(sizeof(DocumentRecord), ARRAYLIST_INIT_CAPACITY);
    if (document_list == NULL) {
        free(tsi);
        return NULL;
    }
    tsi->documents = document_list;

    tsi->map = hashmap_create(HASHMAP_INIT_CAPACITY);
    if (tsi->map == NULL) {
        array_free(document_list);
        free(tsi);
        return NULL;
    }

    tsi->active_docs = 0;
    return tsi;
}

// Returns true if a new token has been parsed
// false signifies that the end of the document has been reached
bool next_token(const char *text, size_t *cursor, Token *token) {
    // Skip leading delimiters (whitespace, punctuation, etc.)
    while (text[*cursor] != '\0' && !is_valid_token_char(text[*cursor])) {
        (*cursor)++;
    }

    if (text[*cursor] == '\0') {
        return false;
    }

    // Save the start of the word
    token->start = &text[*cursor];
    token->byte_offset = *cursor;

    // Iterate until the end of the word then save
    while (text[*cursor] != '\0' && is_valid_token_char(text[*cursor])) {
        (*cursor)++;
    }

    token->length = &text[*cursor] - token->start;
    return true;
}

void tsi_index_document(TextSearchIndex *tsi, const char *name, const char *text) {
    // Initialization!
    DocumentRecord doc;
    doc.doc_id = tsi->documents->count ? *((uint32_t *)array_get(tsi->documents, tsi->documents->count - 1)) + 1 : 1;
    doc.name = strdup(name);
    doc.content_hash = hash(text);
    doc.total_words = 0;
    doc.is_deleted = false;

    size_t cursor = 0;
    Token token;
    while (next_token(text, &cursor, &token)) {
        doc.total_words++;

        // slice the word
        char *word = malloc(sizeof(char) * token.length + 1);
        strncpy(word, token.start, token.length);
        word[token.length] = '\0';

        // Update the hashmap
        // This if else chain looks scary but its just checking whether the word already exists in the hashmap
        TextSearchWordEntry *entry = hashmap_get(tsi->map, word);
        if (entry != NULL) {
            // Word already exists in the hashmap, so just get the pointer and update the bucket
            TextSearchPosting *last_posting = NULL;
            if (entry->matches->count > 0) {
                last_posting = array_get(entry->matches, entry->matches->count - 1);
            }

            if (last_posting != NULL && last_posting->doc_id == doc.doc_id) {
                // Word already has been found once in the document, push the new offset.
                array_push(last_posting->offsets, &token.byte_offset);
            } else {
                // First occurence of the word in this document
                TextSearchPosting new_posting = {.doc_id = doc.doc_id,
                                                 .offsets = array_create(sizeof(size_t), ARRAYLIST_INIT_CAPACITY)};
                array_push(new_posting.offsets, &token.byte_offset);
                array_push(entry->matches, &new_posting);
            }
            free(word);
        } else {
            // First occurrence of this word in the index, needs a whole new entry
            TextSearchWordEntry new_entry = {
                .token_string = word, .matches = array_create(sizeof(TextSearchPosting), ARRAYLIST_INIT_CAPACITY)};
            TextSearchPosting new_posting = {.doc_id = doc.doc_id,
                                             .offsets = array_create(sizeof(size_t), ARRAYLIST_INIT_CAPACITY)};
            array_push(new_posting.offsets, &token.byte_offset);
            array_push(new_entry.matches, &new_posting);
            hashmap_insert(tsi->map, &new_entry);
        }
    }
    array_push(tsi->documents, &doc);
}

TextSearchWordEntry *tsi_search_word(TextSearchIndex *tsi, const char *query) { return hashmap_get(tsi->map, query); }

void tsi_destroy(TextSearchIndex *tsi) {
    if (tsi != NULL) {
        if (tsi->documents != NULL) {
            for (size_t i = 0; i < tsi->documents->count; i++) {
                DocumentRecord *doc = array_get(tsi->documents, i);
                if (doc != NULL && doc->name != NULL) {
                    free(doc->name);
                }
            }
            array_free(tsi->documents);
        }
        hashmap_destroy(tsi->map);
        free(tsi);
    }
}

inline bool is_valid_token_char(char c) {
    if (isalnum((unsigned char)c) || (unsigned char)c == '\'') {
        return true;
    }
    return false;
}
