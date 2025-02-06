/*
 * Statically sized hash table implementation
 * (C) 2012  Sasha Levin <levinsasha928@gmail.com>
 */

#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include <stdlib.h>

#include "array_size.h"
#include "hash.h"
#include "list.h"
#include "log2.h"

// this function to mock ht_create function for testing
void set_ht_create_function(void *(*ht_create_func)(uint32_t));
/**
 * struct hashtable - structure for managing a hash table
 * @table: Pointer to an array of hlist_head structures, representing the buckets of the hash table
 * @bits: The number of bits that determine the size of the hash table
 *
 * This structure encapsulates the essential components of a hash table, including
 * the array of buckets and the size of the hash table. The size is defined in terms
 * of the number of bits, where the actual number of buckets is 2 raised to the power
 * of 'bits'. This allows for a flexible and dynamically sized hash table.
 *
 * The 'table' pointer should point to a dynamically allocated array of hlist_head
 * structures, each of which serves as the head of a linked list for handling hash
 * collisions. The actual hash table size (number of buckets) is computed as 1 << bits.
 *
 * Example Usage:
 *
 * hashtable_t my_hashtable;
 * my_hashtable.table = malloc((1 << my_hashtable.bits) * sizeof(struct hlist_head));
 * my_hashtable.bits = 10; // Example: creating a hash table with 1024 buckets
 *
 * // ... [Operations on the hash table] ...
 *
 * // Free the allocated memory when done
 * free(my_hashtable.table);
 */

typedef struct hashtable {
  struct hlist_head *table; // Pointer to an array of hlist_head, representing the hash table's buckets array
  uint32_t bits;            // Number of bits determining the size of the table
} hashtable_t;

#define DEFINE_HASHTABLE(name, bits)    \
  struct hlist_head name[1 << (bits)] = \
      {[0 ...((1 << (bits)) - 1)] = HLIST_HEAD_INIT}

#define DECLARE_HASHTABLE(name, bits) \
  struct hlist_head name[1 << (bits)]

#define HASH_SIZE(name) (ARRAY_SIZE(name))
#define HASH_BITS(name) ilog2(HASH_SIZE(name))

/**
 * Calculates the remainder of dividing 'num' by a power of two 'divisor'.
 *
 * This function efficiently computes the remainder of the division of 'num' by 'divisor',
 * where 'divisor' is expected to be a power of two. The calculation exploits the fact that
 * for powers of two, the remainder operation can be performed using a bitwise AND operation
 * with 'divisor' - 1. This method is significantly faster than the standard division operation.
 *
 * It is important to ensure that the 'divisor' is indeed a power of two, as the behavior is
 * undefined for non-power-of-two divisors. The function uses an assertion to validate this
 * precondition.
 *
 * @param num The dividend in the division operation.
 * @param divisor The divisor, which must be a power of two.
 * @return The remainder of 'num' divided by 'divisor'.
 */
static inline u32 calc_bkt(u32 num, int divisor) {
  return (u32)num & (divisor - 1);
}

hashtable_t *ht_create(uint32_t bits);

/**
 * CLEAR_HASHTABLE_BITS - safely clear and free all elements in a hash table
 * @tbl: Pointer to the hash table array
 * @bits: The number of bits representing the size of the hash table
 * @struct_type: The type of the structures stored in the hash table
 * @node_member: The name of the hlist_node member within the struct_type
 * @free_func: Function pointer for custom memory deallocation of the struct_type
 *
 * This macro is used for iterating over a hash table, safely removing
 * and deallocating each element contained within. It is designed to be
 * flexible and work with any type of struct that contains an hlist_node.
 *
 * The macro uses the hash_for_each_safe_bits() macro to safely iterate
 * over the hash table, which allows modification (deletion) of the list
 * during iteration. For each element in the hash table, it extracts the
 * user-defined struct from the hlist_node, removes the element from the
 * hash table, and then uses the provided free_func to free the memory
 * associated with it. If free_func is not provided, it causes a compile-time error.
 *
 * This is especially useful for cleaning up a hash table at the end of
 * its usage to ensure that no memory leaks occur.
 *
 * Usage Example:
 *
 * struct my_data_type {
 *     int data;
 *     struct hlist_node my_hlist_node;
 * };
 *
 * void my_data_type_free_func(void *entry) {
 *     // Custom memory deallocation for my_data_type
 *     free(entry);
 * }
 *
 * DECLARE_HASHTABLE(my_table, bits);
 * // ... [Initialization and usage of my_table] ...
 * CLEAR_HASHTABLE_BITS(my_table, bits, struct my_data_type, my_hlist_node, my_data_type_free_func);
 *
 */
#define CLEAR_HASHTABLE_BITS(tbl, bits, struct_type, node_member, free_func) \
  do {                                                                       \
    uint32_t bkt;                                                            \
    struct hlist_node *tmp;                                                  \
    struct_type *cur;                                                        \
    hash_for_each_safe_bits(tbl, bits, bkt, tmp, cur, node_member) {         \
      hash_del(&cur->node_member);                                           \
      free_func(cur);                                                        \
    }                                                                        \
  } while (0)

/**
 * COUNT_ENTRIES_IN_HASHTABLE - count the number of entries in a hash table
 * @ht: Pointer to the hash table
 * @type: The type of the structures stored in the hash table
 * @node: The name of the hlist_node member within the type
 * @count: Variable to store the count of entries
 *
 * This macro is used for iterating over a hash table to count the total number
 * of entries it contains. It leverages the hash_for_each_bits macro to iterate
 * over every bucket and every entry within those buckets in the hash table,
 * incrementing the count for each found entry.
 *
 * The macro is designed to be flexible and can work with any type of struct
 * that contains an hlist_node, allowing for easy integration into existing
 * hash table implementations without requiring specific function implementations
 * for counting.
 *
 * The count result is stored in the variable passed as the @count argument,
 * which must be of size_t type to ensure it can hold the total number of entries
 * without overflow.
 *
 * Note: It is the caller's responsibility to ensure that the hash table and
 * the count variable are properly initialized before using this macro.
 *
 * Usage Example:
 *
 * struct my_struct {
 *     int id;
 *     struct hlist_node node;
 * };
 *
 * hashtable_t my_hashtable; // Assume this is already initialized and populated
 * size_t my_entries_count;
 * COUNT_ENTRIES_IN_HASHTABLE(&my_hashtable, my_struct, node, my_entries_count);
 * printf("The hash table contains %zu entries.\n", my_entries_count);
 *
 */
#define COUNT_ENTRIES_IN_HASHTABLE(ht, type, node, count)     \
  do {                                                        \
    count = 0;                                                \
    uint32_t bkt;                                             \
    type *cur;                                                \
    hash_for_each_bits(ht->table, ht->bits, bkt, cur, node) { \
      count++;                                                \
    }                                                         \
  } while (0)

/**
 * HT_FREE - safely clear and free all elements in a hash table encapsulated within a hashtable_t structure
 * @ht: Pointer to the hashtable_t structure containing the hash table and its size
 * @struct_type: The type of the structures stored in the hash table
 * @node_member: The name of the hlist_node member within the struct_type
 * @free_func: Function pointer for custom memory deallocation of the struct_type
 *
 * This macro is an extension of CLEAR_HASHTABLE_BITS and is specifically designed to be used
 * with a hashtable_t structure. It encapsulates the functionality of CLEAR_HASHTABLE_BITS
 * by automatically providing the hash table and its size from the hashtable_t structure.
 * This macro iterates over the hash table within the hashtable_t structure, safely removing
 * and deallocating each element contained within, using the provided free_func for memory
 * deallocation. If free_func is NULL, standard free() is used.
 *
 * This macro simplifies the process of cleaning up a hash table at the end of its usage,
 * ensuring that no memory leaks occur, and is particularly useful for hashtables with
 * dynamically allocated elements.
 *
 * Usage Example:
 *
 * struct my_data_type {
 *     int data;
 *     struct hlist_node my_hlist_node;
 * };
 *
 * hashtable_t *my_hashtable = create_hashtable(bits, free_string_entry);
 * // ... [ usage of my_hashtable] ...
 *
 * // free all ht entries, ht itself and my_hashtable container.
 * HT_FREE(&my_hashtable, struct my_data_type, my_hlist_node, custom_free_func);
 */
#define HT_FREE(ht, struct_type, node_member, free_func)                                \
  do {                                                                                  \
    CLEAR_HASHTABLE_BITS((ht)->table, (ht)->bits, struct_type, node_member, free_func); \
    free((ht)->table);                                                                  \
    free(ht);                                                                           \
  } while (0)

/**
 * hashtable_add - add an object to a hashtable
 * @ht: Pointer to the hashtable_t structure containing the hash table
 * @node: Pointer to the struct hlist_node representing the object to be added
 * @key: The key associated with the object, used for hashing
 *
 * This macro is used to add an object to a hash table. It calculates the appropriate
 * bucket for the object based on the provided key and the size of the hash table
 * (determined by the number of bits stored in the hashtable_t structure).
 *
 * It is a wrapper around the hash_add_bits macro, leveraging the hashtable_t
 * structure to simplify the process of adding objects to the hash table.
 *
 * Usage Example:
 *
 * struct my_data {
 *     struct hlist_node node;
 *     // Other data fields...
 * };
 *
 * hashtable_t my_hashtable;
 * struct my_data data;
 * unsigned int key = // Compute hash key for data;
 * hashtable_add(&my_hashtable, &data.node, key);
 */
#define hashtable_add(ht, node, key) \
  hash_add_bits((ht)->table, (ht)->bits, node, key)

/**
 * hashtable_del - delete an object from a hashtable
 * @ht: Pointer to the hashtable_t structure containing the hash table
 * @node: Pointer to the struct hlist_node representing the object to be deleted
 *
 * This macro is used to remove an object from a hash table. The object is identified
 * by the provided struct hlist_node. The macro simplifies the deletion process by
 * using the hash table information stored in the hashtable_t structure.
 *
 * It is a wrapper around the hash_del_bits macro and takes care of locating the
 * appropriate bucket from which to remove the object.
 *
 * Usage Example:
 *
 * struct my_data {
 *     struct hlist_node node;
 *     // Other data fields...
 * };
 *
 * hashtable_t my_hashtable;
 * struct my_data data;
 * // Add data to hashtable...
 * hashtable_del(&my_hashtable, &data.node);
 */

#define hashtable_del(ht, node) \
  hash_del_bits((ht)->table, (ht)->bits, node)

static inline void __hash_init(struct hlist_head *ht, unsigned int sz) {
  unsigned int i;

  for (i = 0; i < sz; i++)
    INIT_HLIST_HEAD(&ht[i]);
}

/**
 * hash_init - initialize a hash table
 * @hashtable: hashtable to be initialized
 *
 * Calculates the size of the hashtable from the given parameter, otherwise
 * same as hash_init_size.
 *
 * This has to be a macro since HASH_BITS() will not work on pointers since
 * it calculates the size during preprocessing.
 */
#define hash_init(hashtable) __hash_init(hashtable, HASH_SIZE(hashtable))

/**
 * hashtable_init - initialize a hash table
 * @ht: Pointer to the hashtable_t structure to be initialized
 *
 * This macro initializes a hash table encapsulated within a hashtable_t structure.
 * It sets up each bucket of the hash table, preparing it for use. The number of buckets
 * is determined based on the 'bits' field in the hashtable_t structure.
 *
 * This macro is essentially a wrapper around the __hash_init function, providing a
 * simplified interface for initializing the hash table using the hashtable_t structure.
 *
 * Usage Example:
 *
 * hashtable_t my_hashtable;
 * my_hashtable.table = malloc((1 << bits) * sizeof(struct hlist_head));
 * my_hashtable.bits = bits;
 * hashtable_init(&my_hashtable);
 */
#define hashtable_init(ht) __hash_init((ht)->table, 1 << (ht)->bits)

/**
 * hash_add - add an object to a hashtable
 * @hashtable: hashtable to add to
 * @node: the &struct hlist_node of the object to be added
 * @key: the key of the object to be added
 */
#define hash_add(hashtable, node, key) \
  hlist_add_head(node, &hashtable[calc_bkt(key, 1 << (bits))])

#define hash_add_bits(hashtable, bits, node, key) \
  hlist_add_head(node, &hashtable[calc_bkt(key, 1 << (bits))])

/**
 * hash_hashed - check whether an object is in any hashtable
 * @node: the &struct hlist_node of the object to be checked
 */
static inline int hash_hashed(struct hlist_node *node) {
  return !hlist_unhashed(node);
}

static inline int __hash_empty(struct hlist_head *ht, unsigned int sz) {
  unsigned int i;

  for (i = 0; i < sz; i++)
    if (!hlist_empty(&ht[i]))
      return 0;

  return 1;
}

/**
 * hash_empty - check whether a hashtable is empty
 * @hashtable: hashtable to check
 *
 * This has to be a macro since HASH_BITS() will not work on pointers since
 * it calculates the size during preprocessing.
 */
#define hash_empty(hashtable) __hash_empty(hashtable, HASH_SIZE(hashtable))

/**
 * hash_del - remove an object from a hashtable
 * @node: &struct hlist_node of the object to remove
 */
static inline void hash_del(struct hlist_node *node) {
  hlist_del_init(node);
}

/**
 * hash_for_each - iterate over a hashtable
 * @name: hashtable to iterate
 * @bkt: integer to use as bucket loop cursor
 * @obj: the type * to use as a loop cursor for each entry
 * @member: the name of the hlist_node within the struct
 */
#define hash_for_each(name, bkt, obj, member)                         \
  for ((bkt) = 0, obj = NULL; obj == NULL && (bkt) < HASH_SIZE(name); \
       (bkt)++)                                                       \
  hlist_for_each_entry(obj, &name[bkt], member)

#define hash_for_each_bits(name, bits, bkt, obj, member)                    \
  for ((bkt) = 0, obj = NULL; obj == NULL && (bkt) < (unsigned)(1 << bits); \
       (bkt)++)                                                             \
  hlist_for_each_entry(obj, &name[bkt], member)

/**
 * hash_for_each_safe - iterate over a hashtable safe against removal of
 * hash entry
 * @name: hashtable to iterate
 * @bkt: integer to use as bucket loop cursor
 * @tmp: a &struct used for temporary storage
 * @obj: the type * to use as a loop cursor for each entry
 * @member: the name of the hlist_node within the struct
 */
#define hash_for_each_safe(name, bkt, tmp, obj, member)                 \
  for ((bkt) = 0, obj = NULL; obj == NULL && (bkt) < (HASH_SIZE(name)); \
       (bkt)++)                                                         \
  hlist_for_each_entry_safe(obj, tmp, &name[bkt], member)

#define hash_for_each_safe_bits(name, bits, bkt, tmp, obj, member)          \
  for ((bkt) = 0, obj = NULL; obj == NULL && (bkt) < (unsigned)(1 << bits); \
       (bkt)++)                                                             \
  hlist_for_each_entry_safe(obj, tmp, &name[bkt], member)

/**
 * hash_for_each_possible - iterate over all possible objects hashing to the
 * same bucket
 * @name: hashtable to iterate
 * @obj: the type * to use as a loop cursor for each entry
 * @member: the name of the hlist_node within the struct
 * @key: the key of the objects to iterate over
 */
#define hash_for_each_possible(name, obj, member, key) \
  hlist_for_each_entry(obj, &name[calc_bkt(key, 1 << HASH_BITS(name))], member)

#define hash_for_each_possible_bits(name, hash_bits, obj, member, key) \
  hlist_for_each_entry(obj, &name[calc_bkt(key, 1 << (hash_bits))], member)

/**
 * hash_for_each_possible_safe - iterate over all possible objects hashing to the
 * same bucket safe against removals
 * @name: hashtable to iterate
 * @obj: the type * to use as a loop cursor for each entry
 * @tmp: a &struct used for temporary storage
 * @member: the name of the hlist_node within the struct
 * @key: the key of the objects to iterate over
 */
#define hash_for_each_possible_safe(name, obj, tmp, member, key) \
  hlist_for_each_entry_safe(obj, tmp,                            \
                            &name[calc_bkt(key, 1 << HASH_BITS(name))], member)

#endif
