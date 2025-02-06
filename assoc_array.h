#ifndef ASSOC_ARRAY_H
#define ASSOC_ARRAY_H

#include "hashtable.h" // Include your hashtable header file
#include <stdio.h>
#include <stdlib.h>

typedef struct array_entry {
  struct hlist_node hnode; // Node for hash table linkage
  struct k_list_head lnode;  // Node for doubly linked list
  void *key;
  uint8_t key_size;
  void *data; // Data of the item
} assoc_array_entry_t;

typedef struct array_struct {
  hashtable_t *ht;                                                                        // the hash table
  struct k_list_head list;                                                                  // Head of the doubly linked list for accessing first and last items
  size_t size;                                                                            // Current number of elements in the array
  void (*free_entry)(void *);                                                             // cb function to free entry memory
  int (*fill_entry)(assoc_array_entry_t *entry, void *data, void *key, uint8_t key_size); // cb function to fill entry
} assoc_array_t;

// Functions for array operations
assoc_array_t *
array_create(uint32_t bits, void (*free_entry)(void *),
             int (*fill_entry)(assoc_array_entry_t *entry, void *data, void *key, uint8_t key_size));
int array_free(assoc_array_t *arr);

int array_add(assoc_array_t *arr, void *data, void *key, uint8_t key_size);
int array_add_replace(assoc_array_t *arr, void *data, void *key, uint8_t key_size);
int array_del(assoc_array_t *arr, void *key, uint8_t key_size);

assoc_array_entry_t *array_get_by_key(assoc_array_t *arr, void *key, uint8_t key_size);
assoc_array_entry_t *array_get_head_entry(assoc_array_t *arr);
assoc_array_entry_t *array_get_tail_entry(assoc_array_t *arr);

assoc_array_entry_t *array_get_first(assoc_array_t *arr);
assoc_array_entry_t *array_get_last(assoc_array_t *arr);

int array_del_first(assoc_array_t *arr);
int array_del_last(assoc_array_t *arr);

//this func allow to redefine ht_create
void set_ht_create(hashtable_t *(*ht_create_func)(uint32_t));
#endif // ASSOC_ARRAY_H

int array_collision_percent(const assoc_array_t *arr);