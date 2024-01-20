
#include <stdint.h>
#include <stdlib.h>

#include "hashtable.h"

hashtable_t *create_hashtable(uint32_t bits) {
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

int free_hashtable(hashtable_t *ht) {
  if (!ht) return 1;
  free(ht->table);
  free(ht);
  return 0;
}