#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include <stdbool.h>
#include <string.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "hashtable.h"



h_node_s *get_by_key_first_found(struct ht_hlist_head *tbl, uint8_t hash_bits, uint32_t key){    
    h_node_s *cur;
    /* h_node_s *node; this var is unused */

    hash_for_each_possible_bits(tbl, hash_bits, cur, node, key){
        return cur;
    }

    return NULL;
    
}

h_node_s *get_by_mac_first_found(struct ht_hlist_head *tbl, uint8_t hash_bits, uint8_t mac[IFHWADDRLEN]){    
    h_node_s *cur;
    /* h_node_s *node; this var is unused */
    uint32_t key = hash_time33((const char *)mac, IFHWADDRLEN);

    hash_for_each_possible_bits(tbl, hash_bits, cur, node, key)
    {
        if(memcmp(mac, cur->mac, IFHWADDRLEN) == 0){
            return cur;
        }
    }

    return NULL;
    
}

uint32_t count_by_mac(struct ht_hlist_head *tbl, uint8_t hash_bits, uint8_t mac[IFHWADDRLEN]){    
    h_node_s *cur;
    /* h_node_s *node; this var is unused */
    uint32_t cnt = 0;
    uint32_t key = hash_time33((const char *)mac, IFHWADDRLEN);
    hash_for_each_possible_bits(tbl, hash_bits, cur, node, key){
        if(memcmp(cur->mac, mac, IFHWADDRLEN) == 0) ++cnt;
    }
    return cnt;
    
}

void hashtable_free(struct ht_hlist_head *tbl, uint32_t bits){
    h_node_s *cur, *cur_tmp;
    uint32_t bkt = 0;
    hash_for_each_safe_bits(tbl, bits, bkt, cur, cur_tmp, node) {
        hash_del(&cur_tmp->node);
        free(cur_tmp);
    }
}
