#include "text_search.h"
#include "array_list.h"
#include "builder_hashmap.h"
#include "flatmap.h"
#include "index_builder.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct TextSearchIndex {
    bool is_final;
    union {
        IndexBuilder *builder; // Maps token_string -> BuildEntry and stores index metadata
        FlatIndex *flatmap;    // Flattened, memory optimized immutable version of map
    };
};

static inline bool is_valid_token_char(char c) { return isalnum((unsigned char)c) || (unsigned char)c == '\''; }

// Returns true if a new token has been parsed
// false signifies that the end of the document has been reached
static bool next_token(const char *text, size_t *cursor, Token *token) {
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

static BuildTerm *get_or_create_term(IndexBuilder *builder, const char *word) {
    BuildTerm *term = hashmap_get(builder, word);
    if (term != NULL) {
        // Term has been found before, return it.
        return term;
    }

    // First occurence of the word in the hashmap.
    BuildTerm new_term = {
        .token_string = strdup(word),
        .hash = hash(word),
        .postings = array_create(sizeof(BuildPosting), ARRAYLIST_INIT_CAPACITY),
    };
    builder->total_string_bytes += strlen(word) + 1;

    hashmap_insert(builder, &new_term);
    return hashmap_get(builder, word);
}

static BuildPosting *get_or_create_posting(IndexBuilder *builder, BuildTerm *term, uint32_t doc_id) {
    if (term->postings->count > 0) {
        // Word has been found in this document before
        BuildPosting *posting = array_get(term->postings, term->postings->count - 1);
        if (posting->doc_id == doc_id) {
            return posting;
        }
    }

    // First occurence of the word in this document
    BuildPosting new_posting = {
        .doc_id = doc_id,
        .offsets = array_create(sizeof(size_t), ARRAYLIST_INIT_CAPACITY),
    };
    builder->total_postings++;

    if (new_posting.offsets == NULL) {
        return NULL;
    }

    if (array_push(term->postings, &new_posting)) {
        return NULL;
    }

    return array_get(term->postings, term->postings->count - 1);
}

TextSearchIndex *tsi_create() {
    TextSearchIndex *tsi = malloc(sizeof(TextSearchIndex));
    if (tsi == NULL) {
        return NULL;
    }

    tsi->is_final = false;
    tsi->builder = hashmap_create(HASHMAP_INIT_CAPACITY);
    if (tsi->builder == NULL) {
        free(tsi);
        return NULL;
    }

    return tsi;
}

void tsi_index_document(TextSearchIndex *tsi, const char *name, const char *text) {
    if (tsi->is_final) {
        return;
    }

    // Initialization!
    BuildDoc doc;
    doc.doc_id =
        tsi->builder->docs->count ? *((uint32_t *)array_get(tsi->builder->docs, tsi->builder->docs->count - 1)) + 1 : 1;
    doc.name = strdup(name);
    doc.total_words = 0;

    size_t cursor = 0;
    Token token;
    while (next_token(text, &cursor, &token)) {
        doc.total_words++;

        // Slice word into a stack buffer
        char word[256];
        snprintf(word, sizeof(word), "%.*s", (int)token.length, token.start);

        BuildTerm *term = get_or_create_term(tsi->builder, word);

        BuildPosting *posting = get_or_create_posting(tsi->builder, term, doc.doc_id);

        array_push(posting->offsets, &token.byte_offset);
        tsi->builder->total_offsets++;
    }

    array_push(tsi->builder->docs, &doc);
    tsi->builder->total_doc_name_bytes += strlen(name) + 1;
}

TsiStatus tsi_finalize(TextSearchIndex *tsi) {
    FlatIndex *flatmap = flatmap_create(tsi->builder);
    if (flatmap == NULL) {
        return TSI_ERROR_OOM;
    }

    hashmap_destroy(tsi->builder);

    tsi->flatmap = flatmap;
    tsi->is_final = true;
    return TSI_SUCCESS;
}

TsiStatus tsi_serialize_to_disk(TextSearchIndex *tsi, char *path) {
    if (!tsi->is_final) {
        return TSI_ERROR_NOT_FINALIZED;
    }

    IndexHeader header = {
        .magic = CTEXTSEARCH_MAGIC_NUMBER,
        .version = CTEXTSEARCH_VERSION_NUMBER,

        .string_arena_size = tsi->flatmap->strings_size,
        .doc_count = tsi->flatmap->doc_count,
        .term_capacity = tsi->flatmap->term_capacity,
        .posting_count = tsi->flatmap->posting_count,
        .offset_count = tsi->flatmap->offset_count,
        .total_file_size = sizeof(IndexHeader) + tsi->flatmap->blob_size,

        .strings_offset = (size_t)(tsi->flatmap->strings - (char *)tsi->flatmap->blob),
        .docs_offset = (size_t)((char *)tsi->flatmap->docs - (char *)tsi->flatmap->blob),
        .terms_offset = (size_t)((char *)tsi->flatmap->terms - (char *)tsi->flatmap->blob),
        .postings_offset = (size_t)((char *)tsi->flatmap->postings - (char *)tsi->flatmap->blob),
        .offsets_offset = (size_t)((char *)tsi->flatmap->offsets - (char *)tsi->flatmap->blob),
    };

    FILE *fp = fopen(path, "wb");
    if (!fp)
        return TSI_ERROR_IO;

    if (fwrite(&header, sizeof(IndexHeader), 1, fp) != 1) {
        return TSI_ERROR_IO;
    }

    if (fwrite(tsi->flatmap->blob, tsi->flatmap->blob_size, 1, fp) != 1) {
        return TSI_ERROR_IO;
    }

    fclose(fp);
    return TSI_SUCCESS;
}

TsiStatus tsi_load_from_disk(TextSearchIndex *tsi, char *file_path) {
    if (tsi->is_final) {
        return TSI_ERROR_ALREADY_FINALIZED;
    }

    FILE *fp = fopen(file_path, "rb");
    if (!fp) {
        return TSI_ERROR_IO;
    }

    // Read the header metadata
    IndexHeader header;
    if (fread(&header, sizeof(IndexHeader), 1, fp) != 1) {
        return TSI_ERROR_IO;
    }

    if (header.magic != CTEXTSEARCH_MAGIC_NUMBER || header.version != CTEXTSEARCH_VERSION_NUMBER) {
        return TSI_ERROR_CORRUPT_HEADER;
    }

    FlatIndex *flatmap = malloc(sizeof(FlatIndex));
    if (!flatmap) {
        return TSI_ERROR_OOM;
    }

    // Read the data blob
    size_t blob_size = header.total_file_size - sizeof(IndexHeader);
    void *blob = malloc(blob_size);
    if (fread(blob, blob_size, 1, fp) != 1) {
        return TSI_ERROR_IO;
    }

    // Intialize map
    flatmap->blob_size = blob_size;
    flatmap->doc_count = header.doc_count;
    flatmap->term_capacity = header.term_capacity;
    flatmap->posting_count = header.posting_count;
    flatmap->offset_count = header.offset_count;
    flatmap->strings_size = header.string_arena_size;

    flatmap->docs = blob;
    flatmap->terms = (FlatTerm *)(flatmap->docs + flatmap->doc_count);
    flatmap->postings = (FlatPosting *)(flatmap->terms + flatmap->term_capacity);
    flatmap->offsets = (size_t *)(flatmap->postings + flatmap->posting_count);
    flatmap->strings = (char *)(flatmap->offsets + flatmap->offset_count);

    flatmap->is_mmap = false;

    tsi->flatmap = flatmap;
    tsi->is_final = true;
    return TSI_SUCCESS;
}

TextSearchResult *tsi_search_word(const TextSearchIndex *tsi, const char *query) {
    if (!tsi->is_final) {
        return NULL;
    }

    FlatTerm *term = flatmap_get(tsi->flatmap, query);
    if (term == NULL) {
        return NULL;
    }

    TextSearchResult *result = malloc(sizeof(TextSearchResult));
    if (result == NULL) {
        return NULL;
    }

    result->query = tsi->flatmap->strings + term->string_offset; // Pointer arithmatic to word!
    result->match_count = term->posting_count;
    result->matches = malloc(term->posting_count * sizeof(TextSearchMatch));
    if (result->matches == NULL) {
        free(result);
        return NULL;
    }

    for (size_t i = 0; i < term->posting_count; i++) {
        FlatPosting *posting = &tsi->flatmap->postings[term->posting_start + i];
        FlatDoc *doc = &tsi->flatmap->docs[posting->doc_id - 1];
        result->matches[i] = (TextSearchMatch){
            .doc_id = posting->doc_id,
            .doc_name = tsi->flatmap->strings + doc->name_offset,
            .offsets = tsi->flatmap->offsets + posting->offset_start,
            .offset_count = posting->offset_count,
        };
    }

    return result;
}

void tsi_free_result(TextSearchResult *result) {
    if (result != NULL) {
        free(result->matches);
        free(result);
    }
}

void tsi_destroy(TextSearchIndex *tsi) {
    if (tsi->is_final) {
        flatmap_destroy(tsi->flatmap);
    } else {
        hashmap_destroy(tsi->builder);
    }
    free(tsi);
}
