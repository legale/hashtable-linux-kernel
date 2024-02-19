#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "mactable.h"

mac_node_t *get_by_key_first_found(struct hlist_head *tbl, uint8_t hash_bits, int key) {
  mac_node_t *current;

  hash_for_each_possible_bits(tbl, hash_bits, current, node, key) {
    return current;
  }

  return NULL;
}


mac_node_t *get_by_mac_first_found(struct hlist_head *tbl, uint8_t hash_bits, uint8_t mac[IFHWADDRLEN]) {
  if (tbl == NULL) return NULL;
  mac_node_t *cur;
  int key = hash_time33((const char *)mac, IFHWADDRLEN);

  hash_for_each_possible_bits(tbl, hash_bits, cur, node, key) {
    if (memcmp(mac, cur->mac, IFHWADDRLEN) == 0) {
      return cur;
    }
  }

  return NULL;
}

int count_by_mac(struct hlist_head *tbl, uint8_t hash_bits, uint8_t mac[IFHWADDRLEN]) {
  int cnt = 0;
  int key = hash_time33((const char *)mac, IFHWADDRLEN);
  mac_node_t *cur;

  hash_for_each_possible_bits(tbl, hash_bits, cur, node, key) {
    if (memcmp(cur->mac, mac, IFHWADDRLEN) == 0) {
      ++cnt;
    }
  }
  return cnt;
}



void mactable_free(struct hlist_head *tbl, uint32_t bits) {
    struct hlist_node *tmp;
    uint32_t bkt;
    mac_node_t *cur;

    hash_for_each_safe_bits(tbl, bits, bkt, tmp, cur, node) {
        hash_del(&cur->node);
        if (cur->hostname) free((void *)cur->hostname);
        free(cur);
    }
    free(tbl);
}

