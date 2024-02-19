#ifndef _MACTABLE_H
#define _MACTABLE_H

#include <net/if.h>
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
    uint8_t mac[IFHWADDRLEN];
    const char *hostname; 
} mac_node_s;

mac_node_s *get_by_mac_first_found(struct hlist_head *tbl, uint8_t hash_bits, uint8_t mac[IFHWADDRLEN]);    
int count_by_mac(struct hlist_head *tbl, uint8_t hash_bits, uint8_t mac[IFHWADDRLEN]);
mac_node_s *get_by_key_first_found(struct hlist_head *tbl, uint8_t hash_bits, int key);
void mactable_free(struct hlist_head *tbl, uint32_t bits);

#endif /*_MACTABLE_H */