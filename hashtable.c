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



struct h_node *get_by_key_first_found(struct hlist_head *tbl, uint8_t hash_bits, uint32_t key){    
    struct h_node *cur;
    /* struct h_node *node; this var is unused */

    hash_for_each_possible_bits(tbl, hash_bits, cur, node, key){
        return cur;
    }

    return NULL;
    
}

struct h_node *get_by_mac_first_found(struct hlist_head *tbl, uint8_t hash_bits, uint8_t mac[IFHWADDRLEN]){    
    struct h_node *cur;
    /* struct h_node *node; this var is unused */
    uint32_t key = hash_time33((const char *)mac, IFHWADDRLEN);

    hash_for_each_possible_bits(tbl, hash_bits, cur, node, key)
    {
        if(memcmp(mac, cur->mac, IFHWADDRLEN) == 0){
            return cur;
        }
    }

    return NULL;
    
}

uint32_t count_by_mac(struct hlist_head *tbl, uint8_t hash_bits, uint8_t mac[IFHWADDRLEN]){    
    struct h_node *cur;
    /* struct h_node *node; this var is unused */
    uint32_t cnt = 0;
    uint32_t key = hash_time33((const char *)mac, IFHWADDRLEN);
    hash_for_each_possible_bits(tbl, hash_bits, cur, node, key){
        if(memcmp(cur->mac, mac, IFHWADDRLEN) == 0) ++cnt;
    }
    return cnt;
    
}