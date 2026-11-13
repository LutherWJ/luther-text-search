#ifndef ARRAY_LIST
#define ARRAY_LIST

#include <stddef.h>

#define ARRAYLIST_INIT_CAPACITY 4

typedef struct {
    void *data;
    size_t item_size;
    size_t count;
    size_t capacity;
} ArrayList;

ArrayList *array_create(size_t item_size, size_t initial_capacity);
int array_push(ArrayList *list, const void *item);
void *array_get(const ArrayList *list, size_t index);
void array_free(ArrayList *list);

#endif
