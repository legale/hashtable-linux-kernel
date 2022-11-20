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

static struct h_node *get_by_key_first_found(struct hlist_head *tbl, uint8_t hash_bits, uint32_t key){    
    struct h_node *cur, *node;

    hash_for_each_possible_bits(tbl, hash_bits, cur, node, key)
    {
        return cur;
    }

    return NULL;
    
}

static struct h_node *get_by_mac_first_found(struct hlist_head *tbl, uint8_t hash_bits, uint8_t mac[IFHWADDRLEN]){    
    struct h_node *cur, *node;
    uint32_t key = hash_time33(mac, IFHWADDRLEN);

    hash_for_each_possible_bits(tbl, hash_bits, cur, node, key)
    {
        if(memcmp(mac, cur->mac, IFHWADDRLEN) == 0){
            return cur;
        }
    }

    return NULL;
    
}

static uint32_t count_by_mac(struct hlist_head *tbl, uint8_t hash_bits, uint8_t mac[IFHWADDRLEN]){    
    struct h_node *cur, *node;
    uint32_t cnt = 0;
    uint32_t key = hash_time33(mac, IFHWADDRLEN);
    hash_for_each_possible_bits(tbl, hash_bits, cur, node, key){
        if(memcmp(cur->mac, mac, IFHWADDRLEN) == 0) ++cnt;
    }
    return cnt;
    
}


/* hashtable size */
/* struct hlist_head tbl[256]; */
#define BITS 25
#define DENSITY 0.5 /* entries density factor */
#define PRINT_EACH (uint32_t)((1 << BITS) * DENSITY / 2)
DECLARE_HASHTABLE(tbl, BITS); /* table size 1 << BITS */



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
    int cnt_init = table_size * DENSITY;
    int printer = cnt_init / 4;

    printf("bits shift: %lu size: %lu\n", hash_bits, table_size);

    // Insert the elements.
    int cnt = cnt_init;
    while(cnt--){
        cur = (struct h_node *)malloc(sizeof(struct h_node));
        generate_ipv4(&cur->ip);
        generate_mac(&cur->mac);
        key = hash_time33(cur->mac, IFHWADDRLEN);
        uint32_t bkt_calc = hash_32(key, hash_bits);

        if (cnt % PRINT_EACH == 0){
            uint8_t *m = cur->mac;
            struct in_addr *ip = &cur->ip;
            printf("add: %02X:%02X:%02X:%02X:%02X:%02X %s ", 
                m[0],m[1],m[2],m[3],m[4],m[5], inet_ntoa(*ip)
            );
            printf("bkt: %u k: %u\n", bkt_calc, key); 
        }
        

        hash_add(tbl, &cur->node, key); 


    }
    printf("\n\n");


    // List all elements in the table.
    cnt == cnt_init;
    hash_for_each(tbl, bkt, cur, node) {
        cnt--;
        uint32_t key_calc = hash_time33(cur->mac, IFHWADDRLEN);
        uint32_t bkt_calc = hash_32(key_calc, hash_bits);
        if (cnt % PRINT_EACH == 0){
            uint8_t *m = cur->mac;
            struct in_addr *ip = &cur->ip;
            printf("lst: %02X:%02X:%02X:%02X:%02X:%02X %s ", 
                m[0],m[1],m[2],m[3],m[4],m[5], inet_ntoa(*ip)
            ); 
            printf("bkt: %u k: %u\n", bkt, key_calc); 
        }
    }

    //get first entry found by key
    cur = get_by_key_first_found(tbl, hash_bits, key);
    if (cur) printf("get by key: %u ip: %s\n",  key, inet_ntoa(cur->ip));

    //get first entry found by mac
    cur = get_by_mac_first_found(tbl, hash_bits, cur->mac);
    if (cur) printf("get by mac: %u ip: %s\n",  key, inet_ntoa(cur->ip));


    //cycle again to count entries and count duplicates
    cnt = 0;
    int linked = 0;
    size_t duplicates = 0;
    uint32_t cnt_mac = 0;
    hash_for_each_safe(tbl, bkt, cur, cur_tmp, node) {
        cnt++;
        cnt_mac = count_by_mac(tbl, hash_bits, cur_tmp->mac);
        if (cnt_mac > 1){
            ++duplicates;
            uint8_t *m = cur_tmp->mac;
            struct in_addr *ip = &cur_tmp->ip;
            printf("dup: %02X:%02X:%02X:%02X:%02X:%02X %s\n", 
                m[0],m[1],m[2],m[3],m[4],m[5], inet_ntoa(*ip)
            );
        }
        

        if(cur_tmp->node.next != NULL){         
            linked++;
        }
        hash_del(&cur_tmp->node);
        free(cur_tmp);
    }

    //print results
    printf("dpulicates: %u\n", duplicates);
    printf("cnt: %u collisions: %u perc: %2.2f%\n", cnt, linked, (float)linked / cnt * 100);


    return 0;
}


int main(int argc, char *argv[]) {
    
    
    myhashtable_init();



}