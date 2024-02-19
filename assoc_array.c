#include <string.h> // For memcmp if needed

#include "assoc_array.h"

// Function to create and initialize a new associative array
assoc_array_t *array_create(uint32_t bits, void (*free_entry)(void *)) {
  // Allocate memory for the associative array structure
  assoc_array_t *arr = malloc(sizeof(assoc_array_t));
  if (!arr) {
    perror("Failed to allocate memory for assoc_array");
    return NULL; // Memory allocation failed
  }

  // Create the hash table using the provided ht_create function
  arr->ht = ht_create(bits);
  if (!arr->ht) {
    perror("Failed to create hashtable");
    free(arr);   // Clean up previously allocated memory
    return NULL; // Hashtable creation failed
  }

  // Initialize the list head for the doubly linked list
  K_INIT_LIST_HEAD(&arr->list);

  // Initialize the size
  arr->size = 0;

  // Set the free_entry callback
  arr->free_entry = free_entry;

  return arr; // Return the newly created associative array
}

assoc_array_entry_t *array_get_by_key(assoc_array_t *arr, void *key, uint8_t key_size) {
  // Calculate the hash key and bucket index
  int hash_key = hash_time33(key, key_size); // Assuming hash_time33 is applicable for arbitrary data
  int bkt = calc_bkt(hash_key, 1 << arr->ht->bits);

  struct hlist_node *tmp;
  assoc_array_entry_t *cur;

  // Traverse the linked list at the calculated bucket index
  hlist_for_each_entry_safe(cur, tmp, &arr->ht->table[bkt], hnode) {
    // Assuming you have a way to compare the key stored within `cur`
    // For this, you might need to store the key or its hash within `cur` or have a global way to access keys
    if (memcmp(cur->key, key, key_size) == 0) { // Assuming `cur->key` exists and can be compared
      return cur;                               // Found
    }
  }
  return NULL; // Not found
}

int array_add(assoc_array_t *arr, void *data, void *key, uint8_t key_size) {
  assoc_array_entry_t *new_entry = malloc(sizeof(assoc_array_entry_t));
  if (!new_entry) {
    return -1; // Memory allocation failed
  }

  new_entry->key = malloc(key_size);
  if (!new_entry->key) {
    free(new_entry);
    return -1; // Memory allocation failed for key
  }

  memcpy(new_entry->key, key, key_size); // Copy the key
  new_entry->key_size = key_size;        // Set the key size

  new_entry->data = data; // Assign the data

  int hash_key = hash_time33(key, key_size); // Generate a hash for the key

  hashtable_add(arr->ht, &new_entry->hnode, hash_key); // Add to the hash table

  list_add_tail(&new_entry->lnode, &arr->list); // Add to the end of the list

  arr->size++; // Increment the size

  return 0; // Success
}
int array_del(assoc_array_t *arr, void *key, uint8_t key_size) {
  assoc_array_entry_t *existing_entry = array_get_by_key(arr, key, key_size);

  if (existing_entry == NULL) return 1;

  hlist_del(&existing_entry->hnode);
  list_del(&existing_entry->lnode);
  arr->free_entry(existing_entry); // Free the existing data using the callback
  arr->size--;                     // decrease array size
  return 0;
}

int array_add_replace(assoc_array_t *arr, void *data, void *key, uint8_t key_size) {
  (void)array_del(arr, key, key_size);
  // Delegate the addition of a new entry to a separate function
  return array_add(arr, data, key, key_size);
}

int array_free(assoc_array_t *arr) {
  if (arr == NULL) return -1; // Check if the pointer is NULL

  // Use the HT_FREE macro to free all entries in the hash table
  HT_FREE(arr->ht, assoc_array_entry_t, hnode, arr->free_entry);

  // Finally, free the associative array structure itself
  free(arr);

  return 0; // Successful resource release
}
