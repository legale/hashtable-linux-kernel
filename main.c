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
            "Usage:   %s {BITS_SHIFT} {DENSITY} \n"
            "\n"
            "Example: %s 10 0.5           \n"
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




static void generate_mac(uint8_t *mac){
    for(int i = 0; i < IFHWADDRLEN; i++){
        mac[i] = random() % 256;
    }
}

static void generate_ipv4(struct in_addr *ip){
    ip->s_addr = random() % INT32_MAX;
}



static int myhashtable_init(uint32_t bits, float density){

    //hashtable current node
    struct h_node *cur, *cur_tmp;


    uint32_t printer = (uint32_t)((1 << bits) * density / 2);


    /* 
    * example: struct hlist_head tbl[1 << (bits)]; 
    * DECLARE_HASHTABLE(tbl, bits); 
    */
    //struct hlist_head tbl[1 << bits];

    struct hlist_head *tbl = malloc((1 << bits) * sizeof(struct hlist_head) );


    // Initialize the hashtable.
    //hash_init(tbl);

    __hash_init(tbl, 1 << bits);

  
    //hashtable key
    uint32_t key = 0;

    //hashtable bucket
    uint32_t bkt = 0;




    //get hashtable size
    uint32_t hash_bits = bits;
    uint32_t table_size = 1 << hash_bits;
    int cnt_init = table_size * density;
    printf("bits shift: %u hashtable size: %u\n", hash_bits, table_size);

    // Insert the elements.
    int cnt = cnt_init;
    while(cnt--){
        cur = (struct h_node *)malloc(sizeof(struct h_node));
        generate_ipv4(&cur->ip);
        generate_mac((uint8_t *)&cur->mac);
        key = hash_time33((const char *)cur->mac, IFHWADDRLEN);
        uint32_t bkt_calc = hash_32(key, hash_bits);

        if (cnt % printer == 0){
            uint8_t *m = cur->mac;
            struct in_addr *ip = &cur->ip;
            printf("add: %02X:%02X:%02X:%02X:%02X:%02X %s ", 
                m[0],m[1],m[2],m[3],m[4],m[5], inet_ntoa(*ip)
            );
            printf("bkt: %u k: %u\n", bkt_calc, key); 
        }
        

        hash_add_bits(tbl, bits, &cur->node, key); 


    }
    printf("\n\n");


    // List all elements in the table.
    cnt = cnt_init;
    hash_for_each_bits(tbl, bits, bkt, cur, node) {
        cnt--;
        uint32_t key_calc = hash_time33((const char *)cur->mac, IFHWADDRLEN);
        uint32_t bkt_calc = hash_32(key_calc, hash_bits);
        if (cnt % printer == 0){
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
    //if (cur) printf("get by key: %u ip: %s\n",  key, inet_ntoa(cur->ip));

    //get first entry found by mac
    cur = get_by_mac_first_found(tbl, hash_bits, cur->mac);
    if (cur) printf("get by mac: %u ip: %s\n",  key, inet_ntoa(cur->ip));


    //cycle again to count entries and count duplicates
    cnt = 0;
    int linked = 0;
    size_t duplicates = 0;
    uint32_t cnt_mac = 0;
    hash_for_each_safe_bits(tbl, bits, bkt, cur, cur_tmp, node) {
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
    }


    //remove entries
    hash_for_each_safe_bits(tbl, bits, bkt, cur, cur_tmp, node) {
        hash_del(&cur_tmp->node);
        free(cur_tmp);
    }



    //print results
    printf("duplicates: %lu\n", duplicates / 2);
    printf("cnt: %u collisions: %u perc: %2.2f%%\n", cnt, linked, (float)linked / cnt * 100);


    return 0;
}


int main(int argc, char *argv[]) {
    int bits = 18;
    float density = 0.5;

    /* cli arguments parse */
    argv0 = *argv; /* set program name */
    //if (argc == 1) usage();


    while (argc > 1) {
        NEXT_ARG();
        if (matches(*argv, "-h")) {
            usage();
        } else if (matches(*argv, "--help")) {
            usage();
        } else if (!matches(*argv, "-")) {
            bits = atoi(*argv);
            NEXT_ARG();
            density = atof(*argv);
        } else {
            usage();
        }
        argc--;
        argv++;
    }
    printf("bits: %u density: %f\n", bits, density);
    
    myhashtable_init(bits, density);
    return 0;

}