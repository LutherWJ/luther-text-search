#include "array_list.h"
#include "index_builder.h"
#include "text_search.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_BUFFER_SIZE 512

static const char *get_document_name(const TextSearchIndex *tsi, uint32_t doc_id) {
    if (tsi == NULL || tsi->documents == NULL) {
        return "Unknown";
    }
    for (size_t i = 0; i < tsi->documents->count; i++) {
        DocumentRecord *doc = array_get(tsi->documents, i);
        if (doc != NULL && doc->doc_id == doc_id) {
            return doc->name;
        }
    }
    return "Unknown";
}

int main() {
    TextSearchIndex *tsi = tsi_create();
    struct dirent *entry;
    DIR *dir = opendir("docs");

    if (dir == NULL) {
        printf("Could not open the docs directory.\n"
               "Make sure you launch the application in the root of the project folder.\n");
        return 1;
    }

    printf("Indexing the following files: \n");
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }
        printf("%s\n", entry->d_name);

        // Get file handle
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "docs/%s", entry->d_name);
        FILE *fptr = fopen(filepath, "r");
        if (fptr == NULL) {
            printf("Failed to open file: %s\n", filepath);
            closedir(dir);
            return 1;
        }

        // Allocate memory to hold the text content of the file
        ArrayList *file_buf = array_create(sizeof(char), 4);
        if (file_buf == NULL) {
            return 1;
        }

        // Read file 4 bytes at a time
        int c;
        while ((c = fgetc(fptr)) != EOF) {
            char byte = (char)c;
            if (array_push(file_buf, &byte)) {
                return 1;
            }
        }
        char byte = '\0';
        if (array_push(file_buf, &byte)) {
            return 1;
        }

        tsi_index_document(tsi, entry->d_name, (char *)file_buf->data);
        fclose(fptr);
        array_free(file_buf);
    }
    closedir(dir);

    // Let the users search the documents now!
    bool is_running = true;
    char *input_buf = calloc(512, sizeof(char));
    printf("HINT: Type /quit to exit program.\n");
    while (is_running) {
        // Get input
        printf("Search for a word in the index: ");
        fgets(input_buf, INPUT_BUFFER_SIZE, stdin);
        input_buf[strcspn(input_buf, "\r\n")] = '\0'; // strip trailing newline

        if (strcmp(input_buf, "/quit") == 0) {
            is_running = false;
            break;
        }

        // Perform the search
        TextSearchWordEntry *result = tsi_search_word(tsi, input_buf);

        // Display results:
        if (result == NULL || result->matches == NULL || result->matches->count == 0) {
            printf("No results found.\n");
        } else {
            printf("Showing results for \"%s\" in %zu document(s):\n", result->token_string, result->matches->count);
            for (size_t i = 0; i < result->matches->count; i++) {
                TextSearchPosting *posting = array_get(result->matches, i);
                const char *doc_name = get_document_name(tsi, posting->doc_id);
                printf("Result #%zu:\nDocument Name: %s\n", i + 1, doc_name);
                if (posting->offsets != NULL) {
                    for (size_t j = 0; j < posting->offsets->count; j++) {
                        size_t *offset = array_get(posting->offsets, j);
                        printf(" Starting at character %zu\n", *offset);
                    }
                }
            }
        }
    }

    free(input_buf);
    tsi_destroy(tsi);

    return 0;
}
