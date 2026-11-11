#include "array_list.h"
#include "text_search.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_BUFFER_SIZE 256

static int index_directory(TextSearchIndex *tsi, const char *dir_path) {
    DIR *dir = opendir(dir_path);
    struct dirent *entry;

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
