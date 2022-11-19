#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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


/* name entry to store MAC address */
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

static int myhashtable_init(void)
{
    //hashtable current node
    struct h_node *cur, *node, *cur_tmp;

    /* hashtable size */
    DECLARE_HASHTABLE(tbl, 8);
    /* struct hlist_head tbl[256]; */
    
    //hashtable key
    uint32_t key;

    //hashtable bucket
    uint32_t bkt;

    in_addr_t ipv4;

    // Initialize the hashtable.
    hash_init(tbl);

    int cnt = 60;
    // Insert the elements.
    while(cnt--){
        cur = (struct h_node *)malloc(sizeof(struct h_node));
        generate_ipv4(&cur->ip);
        generate_mac(&cur->mac);
        key = jenkins_one_at_a_time_hash(cur->mac, IFHWADDRLEN);

        printf("a: %02X:%02X:%02X:%02X:%02X:%02X %s %u\n", 
            cur->mac[0],cur->mac[1],cur->mac[2],cur->mac[3],cur->mac[4],cur->mac[5],
        inet_ntoa(cur->ip), key);
        hash_add(tbl, &cur->node, key);
    }


    // List all elements in the table.
    hash_for_each(tbl, bkt, cur, node) {
        uint32_t bkt_calc = hash_32(jenkins_one_at_a_time_hash(cur->mac, IFHWADDRLEN), 8);
        uint32_t key_calc = jenkins_one_at_a_time_hash(cur->mac, IFHWADDRLEN);
        printf("l: %02X:%02X:%02X:%02X:%02X:%02X %s %u %u\n",
            cur->mac[0],cur->mac[1],cur->mac[2],cur->mac[3],cur->mac[4],cur->mac[5], 
            inet_ntoa(cur->ip), bkt_calc, bkt);
    }

    // Get the element with name = "foo".
    hash_for_each_possible(tbl, cur, node, key) {
        printf("last key: %u ip: %s\n", key, inet_ntoa(cur->ip));
        // Remove node
    }

    cnt = 0;
    int linked = 0;
    hash_for_each_safe(tbl, bkt, cur, cur_tmp, node) {
        cnt++;
        if(cur_tmp->node.next != NULL) linked++;
        hash_del(&cur_tmp);
        free(cur_tmp);
    }
    printf("cnt: %u linked: %u\n", cnt, linked);


    return 0;
}


int main(int argc, char *argv[]) {
    
    
    myhashtable_init();




    

}