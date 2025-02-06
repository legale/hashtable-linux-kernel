#include "mock_mem_functions.h"

// Initialize the function pointers to the standard library functions
void *(*custom_malloc)(size_t) = malloc;
void *(*custom_calloc)(size_t, size_t) = calloc;
void *(*custom_realloc)(void *, size_t) = realloc;
void (*custom_free)(void *) = free;

void set_memory_functions(void *(*malloc_func)(size_t), void *(*calloc_func)(size_t, size_t), void *(*realloc_func)(void *, size_t), void (*free_func)(void *)) {
    custom_malloc = malloc_func;
    custom_calloc = calloc_func;
    custom_realloc = realloc_func;
    custom_free = free_func;
}