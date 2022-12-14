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
#include "deque.h" /* linux kernel list based deque structure */

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


static int myhashtable_init(uint32_t bits, float density, float print_freq_density){

    //get hashtable size
    uint32_t hash_bits = bits;
    uint32_t table_size = 1 << hash_bits;
    int cnt_init = table_size * density;
    uint32_t printer = cnt_init * print_freq_density;
    printf("bits shift: %u hashtable size: %u, entries to write: %u print density: %f\n", 
        hash_bits, table_size, cnt_init, print_freq_density);

    //hashtable current node
    h_node_s *cur, *cur_tmp;

    /* 
    * example: struct hlist_head tbl[1 << (bits)]; 
    * DECLARE_HASHTABLE(tbl, bits); 
    */
    //struct hlist_head tbl[1 << bits];

    struct ht_hlist_head *tbl = malloc((1 << bits) * sizeof(struct ht_hlist_head) );


    // Initialize the hashtable.
    //hash_init(tbl);

    __hash_init(tbl, 1 << bits);    
    
    //define deque structure
    DEFINE_DEQ(deq)

  
    //hashtable key
    uint32_t key = 0;

    //hashtable bucket
    uint32_t bkt = 0;



    // Insert the elements.
    int cnt = cnt_init;
    while(cnt--){
        cur = (h_node_s *)malloc(sizeof(h_node_s));
        generate_ipv4(&cur->ip);
        generate_mac((uint8_t *)&cur->mac);
        key = hash_time33((const char *)cur->mac, IFHWADDRLEN);
        uint32_t bkt_calc = hash_32(key, hash_bits);

        if (print_freq_density == 1 || cnt % printer == 0){
            uint8_t *m = cur->mac;
            struct in_addr *ip = &cur->ip;
            printf("add: %02X:%02X:%02X:%02X:%02X:%02X %s ", 
                m[0],m[1],m[2],m[3],m[4],m[5], inet_ntoa(*ip)
            );
            printf("bkt: %u k: %u\n", bkt_calc, key); 
        }
        

        hash_add_bits(tbl, bits, &cur->node, key);
        deq_push_tail(&deq, 5, (void *)cur);
        

    }
    printf("\n\n");


    // List all elements in the table.

    printf("Listing hashtable entries:\n");
    cnt = cnt_init;
    hash_for_each_bits(tbl, bits, bkt, cur, node) {
        cnt--;
        uint32_t key_calc = hash_time33((const char *)cur->mac, IFHWADDRLEN);
        uint32_t bkt_calc = hash_32(key_calc, hash_bits);
        if (print_freq_density == 1 || cnt % printer == 0){
            uint8_t *m = cur->mac;
            struct in_addr *ip = &cur->ip;
            printf("lst: %02X:%02X:%02X:%02X:%02X:%02X %s ", 
                m[0],m[1],m[2],m[3],m[4],m[5], inet_ntoa(*ip)
            ); 
            if(bkt != bkt_calc) printf("warning: bkt != bkt_calc\n");
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

    //DEQ_POP AND PUSH TEST
    {
    deq_s *item;
    deq_pop(&deq, &item);
    h_node_s *node = item->node;
    printf("deq_pop and deq_push popped item again\n");
    deq_push(&deq, 5, (void *)node);
    uint8_t *m = node->mac;
    printf("DEQ_POP popped entry: %02X:%02X:%02X:%02X:%02X:%02X\n",
        m[0],m[1],m[2],m[3],m[4],m[5]);    
    printf("deque new size: %u:\n", deq.size);
    free(item);
    }

    {
    deq_s *item;
    deq_pop_tail(&deq, &item);
    h_node_s *node = item->node;
    uint8_t *m = node->mac;
    printf("DEQ_POP_TAIL popped entry: %02X:%02X:%02X:%02X:%02X:%02X\n",
        m[0],m[1],m[2],m[3],m[4],m[5]);    
    printf("deque new size: %u:\n", deq.size);
    free(item);
    }

    //for each deque entry
    printf("Listing deque entries (%u):\n", deq.size);
    cnt = 0;
    DEQ_FOR_EACH(deq, tmp, list) {
        uint8_t *m = tmp->node->mac;
        struct in_addr *ip = &tmp->node->ip;
        printf("%u %02X:%02X:%02X:%02X:%02X:%02X %s\n", 
            cnt++,
            m[0],m[1],m[2],m[3],m[4],m[5], 
            inet_ntoa(*ip)
        );
    }

    
    //test delete from the middle
    cnt = cnt_init;
    hash_for_each_safe_bits(tbl, bits, bkt, cur, cur_tmp, node) {
        if(cnt-- == cnt_init / 2){
            uint8_t *m = cur_tmp->mac;
            printf("test deletion mac: %02X:%02X:%02X:%02X:%02X:%02X\n",
                m[0],m[1],m[2],m[3],m[4],m[5]);
            cur_tmp = get_by_mac_first_found(tbl, hash_bits, m);
            if (cur_tmp){
                printf("get by mac found: %u ip: %s\n",  key, inet_ntoa(cur_tmp->ip));
                hash_del(&cur_tmp->node);
                free(cur_tmp); 
            }
            break;
        }
    }




    //remove ht entries
    hashtable_free(tbl, bits);
    //remove ht
    free(tbl);

    //remove deque entries
    deq_free(&deq);
    printf("check DEQ_IS_EMPTY: %d\n", deq_isempty(&deq));
    
    //deque itself is on stack no need to free.


    //print results
    printf("duplicates: %lu\n", duplicates / 2);
    printf("cnt: %u collisions: %u perc: %2.2f%%\n", cnt_init, linked, (float)linked / cnt_init * 100);


    return 0;
}


int main(int argc, char *argv[]) {
    int bits = 18;
    float density = 0.5;
    float print_freq_density = 0.5;
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
            NEXT_ARG();
            print_freq_density = atof(*argv);
        } else {
            usage();
        }
        argc--;
        argv++;
    }
    printf("bits: %u density: %f print frequency density: %f\n", bits, density, print_freq_density);
    
    myhashtable_init(bits, density, print_freq_density);
    return 0;

}