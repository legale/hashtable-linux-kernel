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

//this structure to store entries as stack structure
typedef struct deq {
    struct h_node *node;
    struct list_head list;
} deq_s;

typedef struct deq_head {
    uint32_t size;
	struct list_head list;
} deq_head_s;

#define DEFINE_DEQ(name)\
    deq_head_s name;\
    name.size = 0;\
    INIT_LIST_HEAD(&name.list);\

#define DEQ_PUSH_TAIL(name, max_items, entry)\
{\
    if(max_items && name.size == max_items){\
        deq_s *item = list_entry(name.list.next, deq_s, list);\
        list_del(name.list.next);\
        free(item);\
    }else{\
        ++name.size;\
    }\
    deq_s *deq_item = (deq_s *)malloc(sizeof(deq_s));\
    deq_item->node = entry;\
    list_add_tail(&deq_item->list, &name.list);\
}\



#define DEQ_PUSH(name, max_items, entry)\
{\
    if(max_items && name.size == max_items){\
        deq_s *item = list_entry(name.list.next, deq_s, list);\
        list_del(name.list.next);\
        free(item);\
    }else{\
        ++name.size;\
    }\
    deq_s *deq_item = (deq_s *)malloc(sizeof(deq_s));\
    deq_item->node = entry;\
    list_add(&deq_item->list, &name.list);\
}\


#define DEQ_FOR_EACH(name, deq_tmp_name, deq_member)\
    deq_s *deq_tmp_name;\
    list_for_each_entry(deq_tmp_name, &name.list, deq_member)\

#define DEQ_CLEAR(name)\
{\
    deq_s *_tmp_item, *_tmp_item_next;\
    list_for_each_entry_safe(_tmp_item, _tmp_item_next, &name.list, list) {\
        free(_tmp_item);\
    }\
}\

#define DEQ_POP(name, item)\
{\
    item = list_entry(name.list.next, deq_s, list);\
    list_del(name.list.next);\
    --name.size;\
}\

#define DEQ_POP_TAIL(name, item)\
{\
    item = list_entry(name.list.prev, deq_s, list);\
    list_del(name.list.prev);\
    --name.size;\
}\

#define DEQ_ISEMPTY(name) (name.size == 0 ? 1 : 0)


static void _deq_push(deq_head_s *name, bool push_tail_flag, uint32_t max_items, void *entry);
static void deq_push_tail(deq_head_s *name, uint32_t max_items, void *entry);
static void deq_push(deq_head_s *name, uint32_t max_items, void *entry);


static void _deq_push(deq_head_s *name, bool push_tail_flag, uint32_t max_items, void *entry)
{
    if(max_items && name->size == max_items){
        deq_s *item = list_entry(name->list.next, deq_s, list);
        list_del(name->list.next);
        free(item);
    }else{
        ++name->size;
    }
    deq_s *deq_item = (deq_s *)malloc(sizeof(deq_s));
    deq_item->node = entry;

    if(push_tail_flag){
        list_add_tail(&deq_item->list, &name->list);
    }else{
        list_add(&deq_item->list, &name->list);
    }
}

static void deq_push_tail(deq_head_s *name, uint32_t max_items, void *entry)
{
    _deq_push(name, true, max_items, entry);
}


static void deq_push(deq_head_s *name, uint32_t max_items, void *entry)
{
    _deq_push(name, false, max_items, entry);
}


static bool deq_isempty(deq_head_s *name){
    return name->size == 0 ? 1 : 0;
}


static void deq_clear(deq_head_s *name){
    deq_s *_tmp_item, *_tmp_item_next;
    list_for_each_entry_safe(_tmp_item, _tmp_item_next, &name->list, list) {
        free(_tmp_item);
    }
}

static void deq_pop(deq_head_s *name, deq_s **item){
    *item = list_entry(name->list.next, deq_s, list);
    list_del(name->list.next);
    --name->size;
}

static void deq_pop_tail(deq_head_s *name, deq_s **item){
    *item = list_entry(name->list.prev, deq_s, list);
    list_del(name->list.prev);
    --name->size;
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
    struct h_node *cur, *cur_tmp;

    /* 
    * example: struct hlist_head tbl[1 << (bits)]; 
    * DECLARE_HASHTABLE(tbl, bits); 
    */
    //struct hlist_head tbl[1 << bits];

    struct hlist_head *tbl = malloc((1 << bits) * sizeof(struct hlist_head) );


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
        cur = (struct h_node *)malloc(sizeof(struct h_node));
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
        deq_push_tail(&deq, 5, cur);
        

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
    struct h_node *node = item->node;
    printf("deq_pop and deq_push popped item again\n");
    deq_push(&deq, 5, node);
    uint8_t *m = node->mac;
    printf("DEQ_POP popped entry: %02X:%02X:%02X:%02X:%02X:%02X\n",
        m[0],m[1],m[2],m[3],m[4],m[5]);    
    printf("deque new size: %u:\n", deq.size);
    free(item);
    }

    {
    deq_s *item;
    deq_pop_tail(&deq, &item);
    struct h_node *node = item->node;
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




    //remove entries from hashtable
    hash_for_each_safe_bits(tbl, bits, bkt, cur, cur_tmp, node) {
        hash_del(&cur_tmp->node);
        free(cur_tmp);
    }
    free(tbl);


    //remove deque entries
    deq_clear(&deq);
    printf("check DEQ_IS_EMPTY: %d\n", deq_isempty(&deq));
    


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