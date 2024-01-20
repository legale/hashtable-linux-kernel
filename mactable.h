#ifndef _MACTABLE_H
#define _MACTABLE_H

#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "list.h"
#include "hashtable.h"

#ifndef IWINFO_ESSID_MAX_SIZE
#define IWINFO_ESSID_MAX_SIZE 32
#endif

typedef struct mac_node {
    struct hlist_node node;
    struct in_addr ip;
    uint8_t mac[ETH_ALEN];
    const char *hostname; 
} mac_node_s;

mac_node_s *get_by_mac_first_found(struct hlist_head *tbl, uint8_t hash_bits, uint8_t mac[ETH_ALEN]);    
uint32_t count_by_mac(struct hlist_head *tbl, uint8_t hash_bits, uint8_t mac[ETH_ALEN]);
mac_node_s *get_by_key_first_found(struct hlist_head *tbl, uint8_t hash_bits, uint32_t key);
void hashtable_free(struct hlist_head *tbl, uint32_t bits);

#endif /*_MACTABLE_H */