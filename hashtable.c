
#include <stdint.h>
#include <stdlib.h>

#include "hashtable.h"

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
