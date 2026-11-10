#include "array_list.h"
#include <stdlib.h>
#include <string.h>

// Allocate memory and initialize the array list
ArrayList *array_create(size_t item_size, size_t initial_capacity) {
    // capacity lower than 2 is invalid and size cannot be zero
    if (initial_capacity <= 1 || item_size < 1) {
        return NULL;
    }

    ArrayList *list = malloc(sizeof(ArrayList));
    if (list == NULL) {
        return NULL;
    }

    void *items = malloc(item_size * initial_capacity);
    if (items == NULL) {
        free(list);
        return NULL;
    }

    list->count = 0;
    list->capacity = initial_capacity;
    list->item_size = item_size;
    list->data = items;

    return list;
}

// Append an item to the end of the array and grow if needed
int array_push(ArrayList *list, const void *item) {
    // Safety check to prevent segmentation faults
    if (list == NULL || item == NULL) {
        return -1;
    }

    // Check if we are exceeding the current capacity.
    // if so double the size of the array.
    if (list->count >= list->capacity) {
        size_t new_cap = (list->capacity * 2);
        void *new_data = realloc(list->data, new_cap * list->item_size);

        if (new_data == NULL) {
            return -1;
        }

        list->capacity = new_cap;
        list->data = new_data;
    }

    // Copy data into the array
    char *dest = (char *)list->data + (list->count * list->item_size);
    memcpy(dest, item, list->item_size);
    list->count++;

    return 0;
}

// Returns pointer to data at the given index
void *array_get(const ArrayList *list, size_t index) {
    if (list == NULL || index >= list->count) {
        return NULL;
    }
    return (char *)list->data + (index * list->item_size);
}

// cleanup!!!
void array_free(ArrayList *list) {
    if (list != NULL) {
        if (list->data != NULL) {
            free(list->data);
        }
        free(list);
    }
}
