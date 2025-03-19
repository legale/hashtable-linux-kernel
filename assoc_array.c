#include <string.h> // For memcmp if needed
#include <errno.h> // error codes

#ifdef JEMALLOC
#include "jemalloc.h"
#endif
#include "assoc_array.h"
#include "mock_mem_functions.h"

// redefine mem functions with custom version
#define malloc custom_malloc
#define calloc custom_calloc
#define free custom_free

static hashtable_t *(*custom_ht_create)(uint32_t) = ht_create;

#define ht_create custom_ht_create

void set_ht_create(hashtable_t *(*ht_create_func)(uint32_t)){
  custom_ht_create = ht_create_func;
}

static int fill_assoc_array_entry(assoc_array_entry_t *entry, void *data, void *key, uint8_t key_size) {
  entry->key = malloc(key_size);
  if (entry->key) {
    memcpy(entry->key, key, key_size); // Copy the key
    entry->key_size = key_size;        // Set the key size
    entry->data = data;                // Assign the data
    return 0;
  }
  return 1;
}

static void free_assoc_array_entry(void *entry) {
  assoc_array_entry_t *assoc_entry = (assoc_array_entry_t *)entry;
  free(assoc_entry->data); // Free the dynamically allocated data
  free(assoc_entry->key); // Free the dynamically allocated key
  free(assoc_entry); // Free the entry itself
}

// Function to create and initialize a new associative array
assoc_array_t *
array_create(uint32_t bits, void (*free_entry)(void *),
             int (*fill_entry)(assoc_array_entry_t *entry, void *data, void *key, uint8_t key_size)) {
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

  arr->free_entry = free_entry ? free_entry : free_assoc_array_entry;
  arr->fill_entry = fill_entry ? fill_entry : fill_assoc_array_entry;

  return arr; // Return the newly created associative array
}

assoc_array_entry_t *array_get_by_key(assoc_array_t *arr, void *key, uint8_t key_size) {
  if (!arr) return NULL;
  // Calculate the hash key and bucket index
  u32 hash_key = hash32_str(key, key_size);
  u32 bkt = calc_bkt(hash_key, 1 << arr->ht->bits);

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
  if (!arr) return -1;
  assoc_array_entry_t *new_entry = malloc(sizeof(assoc_array_entry_t));
  if (!new_entry) {
    perror("malloc for the new_entry failed");
    return -1; // Memory allocation failed
  }

  int ret = arr->fill_entry(new_entry, data, key, key_size);
  if (ret) {
    perror("fill_entry failed");
    free(new_entry);
    return -1; // Memory allocation failed
  }

  int hash_key = hash32_str(key, key_size);           // Generate a hash for the key
  hashtable_add(arr->ht, &new_entry->hnode, hash_key); // Add to the hash table
  k_list_add_tail(&new_entry->lnode, &arr->list);      // Add to the end of the list
  arr->size++;                                         // Increment the size

  return 0; // Success
}

int array_del(assoc_array_t *arr, void *key, uint8_t key_size) {
  if (!arr) return EINVAL;
  assoc_array_entry_t *existing_entry = array_get_by_key(arr, key, key_size);

  if (existing_entry == NULL) return 1;

  hlist_del(&existing_entry->hnode);
  k_list_del(&existing_entry->lnode);
  arr->free_entry(existing_entry); // Free the existing data using the callback
  arr->size--;                     // decrease array size
  return 0;
}

int array_add_replace(assoc_array_t *arr, void *data, void *key, uint8_t key_size) {
  if (!arr) return -1;
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

static assoc_array_entry_t *_array_get_first(assoc_array_t *arr, bool is_first) {
  if (arr == NULL || k_list_empty(&arr->list) || arr->size == 0) return NULL;
  if (is_first) {
    return k_list_first_entry(&arr->list, assoc_array_entry_t, lnode);
  } else {
    return k_list_last_entry(&arr->list, assoc_array_entry_t, lnode);
  }
}

assoc_array_entry_t *array_get_first(assoc_array_t *arr) {
  return _array_get_first(arr, true);
}

assoc_array_entry_t *array_get_last(assoc_array_t *arr) {
  return _array_get_first(arr, false);
}

static int _array_del_first(assoc_array_t *arr, bool is_first) {
  if (arr == NULL || k_list_empty(&arr->list) || arr->size == 0) return -1;
  assoc_array_entry_t *e;
  if (is_first) {
    e = k_list_first_entry(&arr->list, assoc_array_entry_t, lnode);
  } else {
    e = k_list_last_entry(&arr->list, assoc_array_entry_t, lnode);
  }
  if (e == NULL) return -1;

  // delete from ht and list
  hlist_del(&e->hnode);
  k_list_del(&e->lnode);

  // free entry and decrease size
  arr->free_entry(e);
  arr->size--;

  return 0;
}

int array_del_first(assoc_array_t *arr) {
  return _array_del_first(arr, true);
}

int array_del_last(assoc_array_t *arr) {
  return _array_del_first(arr, false);
}

int array_collision_percent(const assoc_array_t *arr) {
    if (!arr || !arr->ht || arr->size == 0) {
        return 0;
    }

    uint32_t num_buckets = 1 << arr->ht->bits; 
    size_t collisions = 0;

    for (uint32_t i = 0; i < num_buckets; i++) {
        struct hlist_head *head = &arr->ht->table[i];
        size_t count = 0;
        assoc_array_entry_t *cur;

        hlist_for_each_entry(cur, head, hnode) {
            count++;
        }

        if (count > 1) {
            collisions += (count - 1);
        }
    }

    return (int)((collisions * 100) / arr->size);
}