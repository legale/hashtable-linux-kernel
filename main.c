#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include <stdbool.h>
#include <string.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "hashtable.h" /* linux kernel hashtable */

/* cli arguments parse macro and functions */
#define NEXT_ARG() do { argv++; if (--argc <= 0) incomplete_command(); } while(0)
#define NEXT_ARG_OK() (argc - 1 > 0)
#define PREV_ARG() do { argv--; argc++; } while(0)
static char *argv0; /* ptr to the program name string */


static void incomplete_command(void) {
    fprintf(stderr, "Command line is not complete. Try -h or --help\n");
    exit(-1);
}

static void usage(void) {
    fprintf(stdout,
            "Usage:   %s {INTERFACE}  \n"
            "\n"
            "Example: %s eth0           \n"
            "\n", argv0, argv0);
    exit(-1);
}

/* Returns true if 'prefix' is a not empty prefix of 'string'. */
static bool matches(const char *prefix, const char *string) {
    if (!*prefix)
        return false;
    while (*string && *prefix == *string) {
        prefix++;
        string++;
    }
    return !*prefix;
}



/********************************/


struct h_node {
    uint32_t name;
    struct in_addr ip;
    uint8_t mac[IFHWADDRLEN];
    struct hlist_node node;
} __attribute__ ((__packed__));



static void generate_mac( uint8_t mac[IFHWADDRLEN]){
    for(int i = 0; i < IFHWADDRLEN; i++){
        mac[i] = random() % 256;
    }
}

static void generate_ipv4(struct in_addr *ip){
    ip->s_addr = random() % INT32_MAX;
}

static struct h_node *get_first_found(struct hlist_head *tbl, uint8_t hash_bits, uint32_t key){    
    struct h_node *cur, *node;

    hash_for_each_possible_bits(tbl, hash_bits, cur, node, key)
    {
        return cur;
    }

    return NULL;
    
}


/* hashtable size */
/* struct hlist_head tbl[256]; */
DECLARE_HASHTABLE(tbl, 24);  


static int myhashtable_init(void){
    //hashtable current node
    struct h_node *cur, *node, *cur_tmp, *cur_tmp2;

  
    //hashtable key
    uint32_t key;

    //hashtable bucket
    uint32_t bkt;

    in_addr_t ipv4;

    // Initialize the hashtable.
    hash_init(tbl);

    //get hashtable size
    uint32_t hash_bits = HASH_BITS(tbl);
    uint32_t table_size = 1 << hash_bits;
    int cnt_init = table_size / 5;
    int printer = cnt_init / 4;

    printf("bits shift: %lu size: %lu\n", hash_bits, table_size);

    // Insert the elements.
    size_t duplicates = 0;
    int cnt = cnt_init;
    while(cnt--){
        cur = (struct h_node *)malloc(sizeof(struct h_node));
        generate_ipv4(&cur->ip);
        generate_mac(&cur->mac);
        key = hash_time33(cur->mac, IFHWADDRLEN);

        if (cnt % printer == 0) printf("a: %02X:%02X:%02X:%02X:%02X:%02X %s k: %u\n", 
            cur->mac[0],cur->mac[1],cur->mac[2],cur->mac[3],cur->mac[4],cur->mac[5],
        inet_ntoa(cur->ip), key);
        
        

        hash_add(tbl, &cur->node, key); 


    }
    printf("duplicates: %lu\n", duplicates);


    // List all elements in the table.
    cnt == cnt_init;
    hash_for_each(tbl, bkt, cur, node) {
        cnt--;
        uint32_t key_calc = hash_time33(cur->mac, IFHWADDRLEN);
        uint32_t bkt_calc = hash_32(key_calc, hash_bits);
        if (cnt % printer == 0) printf("l: %02X:%02X:%02X:%02X:%02X:%02X %s bkt_calc/bkt: %u/%u\n",
            cur->mac[0],cur->mac[1],cur->mac[2],cur->mac[3],cur->mac[4],cur->mac[5], 
            inet_ntoa(cur->ip), bkt_calc, bkt);
    }


    cur = get_first_found(tbl, hash_bits, key);
    if (cur) printf("last key: %u ip: %s\n",  key, inet_ntoa(cur->ip));
    

    cnt = 0;
    int linked = 0;
    hash_for_each_safe(tbl, bkt, cur, cur_tmp, node) {
        cnt++;
        if(cur_tmp->node.next != NULL) linked++;
        hash_del(&cur_tmp->node);
        free(cur_tmp);
    }
    
    printf("cnt: %u linked: %u perc: %2.2f%\n", cnt, linked, (float)linked / cnt * 100);


    return 0;
}


int main(int argc, char *argv[]) {
    
    
    myhashtable_init();



}