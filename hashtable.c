
#include <stdint.h>
#include <stdlib.h>

#ifdef JEMALLOC
#include "jemalloc.h"
#endif
#include "hashtable.h"
#include "mock_mem_functions.h"

// redefine mem functions with custom version
#define malloc custom_malloc
#define calloc custom_calloc
#define free custom_free

hashtable_t *ht_create(uint32_t bits) {
  hashtable_t *ht = malloc(sizeof(hashtable_t));
  if (!ht) return NULL;

  ht->bits = bits;
  ht->table = malloc((1 << bits) * sizeof(struct hlist_head));
  if (!ht->table) {
    free(ht);
    return NULL;
  }

  hashtable_init(ht);
  return ht;
}
