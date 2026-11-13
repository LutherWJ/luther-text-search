#include "text_search.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_BUFFER_SIZE 256

static size_t get_file_size(FILE *fp) {
    fseek(fp, 0L, SEEK_END); // Jump to the end of the file
    long size = ftell(fp);   // Get current byte position
    rewind(fp);              // Reset cursor back to start of file
    return size;
}

static int index_directory(TextSearchIndex *tsi, const char *dir_path) {
    // Get handle to the docs directory
    DIR *dir = opendir(dir_path);
    struct dirent *entry;

    if (dir == NULL) {
        printf("Could not open the docs directory.\n"
               "Make sure you launch the application in the root of the project folder.\n");
        return 1;
    }

    printf("Indexing the following files: \n");
    // Sequentially read and index each file in the directory
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }
        printf("%s\n", entry->d_name);

        // Get file handle
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "docs/%s", entry->d_name);
        FILE *fp = fopen(filepath, "r");
        if (fp == NULL) {
            printf("Failed to open file: %s\n", filepath);
            closedir(dir);
            return 1;
        }

        // Allocate memory to hold the text content of the file
        size_t file_size = get_file_size(fp);
        char *file_buf = malloc(file_size);
        if (file_buf == NULL) {
            return 1;
        }

        // Read file 4 bytes at a time
        char c;
        for (size_t i = 0; i < file_size; i++) {
            file_buf[i] = (char)fgetc(fp);
        }

        char byte = '\0';

        tsi_index_document(tsi, entry->d_name, file_buf);
        fclose(fp);
        free(file_buf);
    }
    closedir(dir);
    return 0;
}

static int query_user_search(const TextSearchIndex *tsi) {
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
        TextSearchResult *result = tsi_search_word(tsi, input_buf);

        // Print Results
        if (result == NULL) {
            printf("No results found.\n");
        } else {
            printf("Showing results for \"%s\" in %zu doc(s):\n", result->query, result->match_count);
            for (size_t i = 0; i < result->match_count; i++) {
                TextSearchMatch *m = &result->matches[i];
                printf("Result #%zu: Document: %s\n", i + 1, m->doc_name);
                for (size_t j = 0; j < m->offset_count; j++) {
                    printf("  Character %lu\n", m->offsets[j]);
                }
            }
            tsi_free_result(result);
        }
    }
    free(input_buf);
    return 0;
}

int main() {
    TextSearchIndex *tsi = tsi_create();

    if (index_directory(tsi, "docs") != 0) {
        return 1;
    }

    tsi_finalize(tsi);

    if (query_user_search(tsi) != 0) {
        return 1;
    }

    tsi_destroy(tsi);
    return 0;
}
