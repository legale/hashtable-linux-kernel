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


mac_node_s *get_by_mac_first_found(struct hlist_head *tbl, uint8_t hash_bits, uint8_t mac[ETH_ALEN]) {
  mac_node_s *cur;
  if (tbl == NULL) return NULL;
  uint32_t key = hash_time33((const char *)mac, ETH_ALEN);

  hash_for_each_possible_bits(tbl, hash_bits, cur, node, key) {
    if (memcmp(mac, cur->mac, ETH_ALEN) == 0) {
      return cur;
    }
  }

  return NULL;
}

uint32_t count_by_mac(struct hlist_head *tbl, uint8_t hash_bits, uint8_t mac[ETH_ALEN]) {
  mac_node_s *cur;
  /* h_node_s *node; this var is unused */
  uint32_t cnt = 0;
  uint32_t key = hash_time33((const char *)mac, ETH_ALEN);
  hash_for_each_possible_bits(tbl, hash_bits, cur, node, key) {
    if (memcmp(cur->mac, mac, ETH_ALEN) == 0) ++cnt;
  }
  return cnt;
}

void hashtable_free(struct hlist_head *tbl, uint32_t bits) {
  mac_node_s *cur, *cur_tmp;
  uint32_t bkt = 0;
  hash_for_each_safe_bits(tbl, bits, bkt, cur, cur_tmp, node) {
    hash_del(&cur_tmp->node);
    free(cur_tmp);
  }
}
