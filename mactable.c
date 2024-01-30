#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "mactable.h"

mac_node_s *get_by_key_first_found(struct hlist_head *tbl, uint8_t hash_bits, uint32_t key) {
  mac_node_s *current;

  hash_for_each_possible_bits(tbl, hash_bits, current, node, key) {
    return current;
  }

  return NULL;
}


mac_node_s *get_by_mac_first_found(struct hlist_head *tbl, uint8_t hash_bits, uint8_t mac[IFHWADDRLEN]) {
  if (tbl == NULL) return NULL;
  mac_node_s *cur;
  uint32_t key = hash_time33((const char *)mac, IFHWADDRLEN);

  hash_for_each_possible_bits(tbl, hash_bits, cur, node, key) {
    if (memcmp(mac, cur->mac, IFHWADDRLEN) == 0) {
      return cur;
    }
  }

  return NULL;
}

uint32_t count_by_mac(struct hlist_head *tbl, uint8_t hash_bits, uint8_t mac[IFHWADDRLEN]) {
  uint32_t cnt = 0;
  uint32_t key = hash_time33((const char *)mac, IFHWADDRLEN);
  mac_node_s *cur;

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
    mac_node_s *cur;

    hash_for_each_safe_bits(tbl, bits, bkt, tmp, cur, node) {
        hash_del(&cur->node);
        if (cur->hostname) free((void *)cur->hostname);
        free(cur);
    }
    free(tbl);
}

