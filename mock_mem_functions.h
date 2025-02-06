#ifndef MOCK_MEM_FUNCTIONS_H
#define MOCK_MEM_FUNCTIONS_H

#include <stdlib.h>

// Function pointers declarations
extern void *(*custom_malloc)(size_t);
extern void *(*custom_calloc)(size_t, size_t);
extern void (*custom_free)(void *);

// Function to set custom memory functions
void set_memory_functions(void *(*malloc_func)(size_t), void *(*calloc_func)(size_t, size_t), void *(*realloc_func)(void *, size_t), void (*free_func)(void *));

#endif // MOCK_MEM_FUNCTIONS_H
