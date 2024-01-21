/*
 * Statically sized hash table implementation
 * (C) 2012  Sasha Levin <levinsasha928@gmail.com>
 */

#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include "list.h"
#include "hash.h"
#include "log2.h"
#include "array_size.h"


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

#define DEFINE_HASHTABLE(name, bits)						\
	struct hlist_head name[1 << (bits)] =					\
			{ [0 ... ((1 << (bits)) - 1)] = HT_HLIST_HEAD_INIT }

#define DECLARE_HASHTABLE(name, bits)                                   	\
	struct hlist_head name[1 << (bits)]

#define HASH_SIZE(name) (ARRAY_SIZE(name))
#define HASH_BITS(name) ilog2(HASH_SIZE(name))


/**
 * create_hashtable - Create and initialize a new hash table
 * @bits: Number of bits to determine the size of the hash table
 *
 * This function allocates memory for a new hashtable_t structure and its associated
 * hash table array. The size of the hash table is determined by the 'bits' parameter,
 * with the actual number of buckets being 2 raised to the power of 'bits'.
 *
 * The function initializes each bucket of the hash table to be an empty list, preparing
 * it for use. In case of memory allocation failure, the function returns NULL.
 *
 * Usage Example:
 * 
 * // Create a hash table with 1024 buckets (2^10)
 * hashtable_t *my_hashtable = create_hashtable(10);
 * 
 * // ... [Operations on the hash table] ...
 * 
 * // Cleanup
 * free_hashtable(my_hashtable); // Assuming free_hashtable is a function you've defined to deallocate the hash table
 *
 * Return: On success, a pointer to the newly created hashtable_t structure.
 *         On failure, NULL.
 */
hashtable_t *create_hashtable(uint32_t bits);
int free_hashtable(hashtable_t *ht);

/**
 * CLEAR_HASHTABLE_BITS - safely clear and free all elements in a hash table
 * @tbl: Pointer to the hash table array
 * @bits: The number of bits representing the size of the hash table
 * @struct_type: The type of the structures stored in the hash table
 * @node_member: The name of the hlist_node member within the struct_type
 *
 * This macro is used for iterating over a hash table, safely removing
 * and deallocating each element contained within. It is designed to be
 * flexible and work with any type of struct that contains an hlist_node.
 * 
 * The macro uses the hash_for_each_safe_bits() macro to safely iterate
 * over the hash table, which allows modification (deletion) of the list
 * during iteration. For each element in the hash table, it extracts the
 * user-defined struct from the hlist_node, removes the element from the
 * hash table, and then frees the memory associated with it.
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
 * DECLARE_HASHTABLE(my_table, bits);
 * // ... [Initialization and usage of my_table] ...
 * CLEAR_HASHTABLE_BITS(my_table, bits, struct my_data_type, my_hlist_node);
 */
#define CLEAR_HASHTABLE_BITS(tbl, bits, struct_type, node_member) do { \
    uint32_t bkt; \
    struct hlist_node *tmp; \
    struct_type *cur; \
    hash_for_each_safe_bits(tbl, bits, bkt, tmp, cur, node_member) { \
        hash_del(&cur->node_member); \
        free(cur); \
    } \
} while (0)


/**
 * CLEAR_HASHTABLE - safely clear and free all elements in a hash table
 * @ht: Pointer to the hashtable_t structure containing the hash table and its size
 * @struct_type: The type of the structures stored in the hash table
 * @node_member: The name of the hlist_node member within the struct_type
 *
 * This macro is used for iterating over a hash table encapsulated within a hashtable_t
 * structure, safely removing and deallocating each element contained within. It is
 * designed to work with any type of struct that contains an hlist_node.
 * 
 * The macro uses the hash_for_each_safe_bits() macro to safely iterate over the hash
 * table, allowing modification (deletion) of the list during iteration. For each element
 * in the hash table, it extracts the user-defined struct from the hlist_node, removes
 * the element from the hash table, and then frees the memory associated with it.
 *
 * This is particularly useful for cleaning up a hash table at the end of its usage,
 * ensuring that no memory leaks occur.
 *
 * Usage Example:
 * 
 * struct my_data_type {
 *     int data;
 *     struct hlist_node my_hlist_node;
 * };
 *
 * hashtable_t my_hashtable;
 * // ... [Initialization and usage of my_hashtable] ...
 * CLEAR_HASHTABLE(&my_hashtable, struct my_data_type, my_hlist_node);
 */
#define CLEAR_HASHTABLE(ht, struct_type, node_member) do { \
  CLEAR_HASHTABLE_BITS((ht)->table, (ht)->bits, struct_type, node_member); \
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


static inline void __hash_init(struct hlist_head *ht, unsigned int sz)
{
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
#define hash_add(hashtable, node, key)						\
	hlist_add_head(node, &hashtable[hash_32(key, HASH_BITS(hashtable))])

#define hash_add_bits(hashtable, bits, node, key)						\
	hlist_add_head(node, &hashtable[hash_32(key, bits)])


/**
 * hash_hashed - check whether an object is in any hashtable
 * @node: the &struct hlist_node of the object to be checked
 */
static inline int hash_hashed(struct hlist_node *node)
{
	return !hlist_unhashed(node);
}

static inline int __hash_empty(struct hlist_head *ht, unsigned int sz)
{
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
static inline void hash_del(struct hlist_node *node)
{
	hlist_del_init(node);
}

/**
 * hash_for_each - iterate over a hashtable
 * @name: hashtable to iterate
 * @bkt: integer to use as bucket loop cursor
 * @obj: the type * to use as a loop cursor for each entry
 * @member: the name of the hlist_node within the struct
 */
#define hash_for_each(name, bkt, obj, member)				\
	for ((bkt) = 0, obj = NULL; obj == NULL && (bkt) < HASH_SIZE(name);\
			(bkt)++)\
		hlist_for_each_entry(obj, &name[bkt], member)

#define hash_for_each_bits(name, bits, bkt, obj, member)				\
	for ((bkt) = 0, obj = NULL; obj == NULL && (bkt) < (unsigned)(1 << bits);\
			(bkt)++)\
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
#define hash_for_each_safe(name, bkt, tmp, obj, member)			\
	for ((bkt) = 0, obj = NULL; obj == NULL && (bkt) < (HASH_SIZE(name));\
			(bkt)++)\
		hlist_for_each_entry_safe(obj, tmp, &name[bkt], member)

#define hash_for_each_safe_bits(name, bits, bkt, tmp, obj, member)			\
	for ((bkt) = 0, obj = NULL; obj == NULL && (bkt) < (unsigned)(1 << bits);\
			(bkt)++)\
		hlist_for_each_entry_safe(obj, tmp, &name[bkt], member)		

/**
 * hash_for_each_possible - iterate over all possible objects hashing to the
 * same bucket
 * @name: hashtable to iterate
 * @obj: the type * to use as a loop cursor for each entry
 * @member: the name of the hlist_node within the struct
 * @key: the key of the objects to iterate over
 */
#define hash_for_each_possible(name, obj, member, key)			\
	hlist_for_each_entry(obj, &name[hash_32(key, HASH_BITS(name))], member)

#define hash_for_each_possible_bits(name, hash_bits, obj, member, key)			\
	hlist_for_each_entry(obj, &name[hash_32(key, hash_bits)], member)


/**
 * hash_for_each_possible_safe - iterate over all possible objects hashing to the
 * same bucket safe against removals
 * @name: hashtable to iterate
 * @obj: the type * to use as a loop cursor for each entry
 * @tmp: a &struct used for temporary storage
 * @member: the name of the hlist_node within the struct
 * @key: the key of the objects to iterate over
 */
#define hash_for_each_possible_safe(name, obj, tmp, member, key)	\
	hlist_for_each_entry_safe(obj, tmp,\
		&name[hash_32(key, HASH_BITS(name))], member)


#endif
