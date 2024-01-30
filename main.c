#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <string.h>

#include "deque.h"     /* linux kernel list based deque structure */
#include "hashtable.h" /* linux kernel hashtable */
#include "list.h"      /* linux kernel linked list */
#include "mactable.h"  /* mac address hashtable */

/* cli arguments parse macro and functions */
#define NEXT_ARG()                         \
  do {                                     \
    argv++;                                \
    if (--argc <= 0) incomplete_command(); \
  } while (0)
#define NEXT_ARG_OK() (argc - 1 > 0)
#define PREV_ARG() \
  do {             \
    argv--;        \
    argc++;        \
  } while (0)
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
          "\n",
          argv0, argv0);
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

static void generate_mac(uint8_t *mac) {
  for (int i = 0; i < IFHWADDRLEN; i++) {
    mac[i] = random() % 256;
  }
}

static void generate_ipv4(struct in_addr *ip) {
  ip->s_addr = random() % INT32_MAX;
}

static int do_hashtable_stuff(uint8_t bits, float density, float print_freq_density) {
  printf("%s\n", __func__);
  // get hashtable size
  uint32_t hash_bits = bits;
  uint32_t table_size = 1 << hash_bits;
  int cnt_init = table_size * density;
  uint32_t printer = cnt_init * print_freq_density;
  printf("bits shift: %u hashtable size: %u, entries to write: %u print density: %f\n",
         hash_bits, table_size, cnt_init, print_freq_density);

  // hashtable current node and tmp var for use with _safe macro
  struct hlist_node *tmp;
  mac_node_s *cur;

  /*
   * example: struct hlist_head tbl[1 << (bits)];
   * DECLARE_HASHTABLE(tbl, bits);
   */
  // struct hlist_head tbl[1 << bits];

  struct hlist_head *tbl = malloc((1 << bits) * sizeof(struct hlist_head));

  // Initialize the hashtable.
  // __hash_init(tbl, 1 << bits); same as hash_init(tbl)
  // but this works with array not a heap allocated hashtabled
  __hash_init(tbl, 1 << bits);

  // define deque structure
  DEFINE_DEQ(deq)

  // hashtable key
  uint32_t key = 0;

  // hashtable bucket
  uint32_t bkt = 0;

  // Insert the elements.
  int cnt = cnt_init;
  while (cnt--) {
    cur = (mac_node_s *)calloc(1, sizeof(mac_node_s));
    generate_ipv4(&cur->ip);
    generate_mac((uint8_t *)&cur->mac);
    key = hash_time33((const char *)cur->mac, IFHWADDRLEN);
    uint32_t bkt_calc = calc_bkt(key, 1 << hash_bits);

    if (cnt % printer == 0) {
      uint8_t *m = cur->mac;
      struct in_addr *ip = &cur->ip;
      printf("add: %02X:%02X:%02X:%02X:%02X:%02X %s ",
             m[0], m[1], m[2], m[3], m[4], m[5], inet_ntoa(*ip));
      printf("bkt: %u k: %u\n", bkt_calc, key);
    }

    hash_add_bits(tbl, bits, &cur->node, key);
    deq_push_tail(&deq, 5, (void *)cur);
  }
  printf("\n\n");

  // List all elements in the table.

  printf("Listing hashtable entries:\n");
  cnt = cnt_init;
  hash_for_each_bits(tbl, hash_bits, bkt, cur, node) {
    cnt--;
    uint32_t key_calc = hash_time33((const char *)cur->mac, IFHWADDRLEN);
    uint32_t bkt_calc = calc_bkt(key_calc, 1 << hash_bits);
    if (cnt % printer == 0) {
      uint8_t *m = cur->mac;
      struct in_addr *ip = &cur->ip;
      printf("lst: %02X:%02X:%02X:%02X:%02X:%02X %s ",
             m[0], m[1], m[2], m[3], m[4], m[5], inet_ntoa(*ip));
      if (bkt != bkt_calc) printf("warning: bkt != bkt_calc\n");
      printf("bkt: %u k: %u\n", bkt, key_calc);
    }
  }

  // get first entry found by key
  cur = get_by_key_first_found(tbl, hash_bits, key);
  if (cur) printf("get by key: %u ip: %s\n", key, inet_ntoa(cur->ip));

  // get first entry found by mac
  cur = get_by_mac_first_found(tbl, hash_bits, cur->mac);
  if (cur) printf("get by mac: %u ip: %s\n", key, inet_ntoa(cur->ip));

  // count entries with specified mac
  {
    uint32_t cnt_mac = count_by_mac(tbl, hash_bits, cur->mac);
    uint8_t *m = cur->mac;
    printf("mac: %02X:%02X:%02X:%02X:%02X:%02X\n", m[0], m[1], m[2], m[3], m[4], m[5]);
    printf("cnt: %u\n", cnt_mac);
  }

  // DEQ_POP AND PUSH TEST
  {
    deq_entry_s *item;
    deq_pop(&deq, &item);
    mac_node_s *node = item->data;
    printf("deq_pop and deq_push popped item again\n");
    deq_push(&deq, 5, (void *)node);
    uint8_t *m = node->mac;
    printf("DEQ_POP popped entry: %02X:%02X:%02X:%02X:%02X:%02X\n",
           m[0], m[1], m[2], m[3], m[4], m[5]);
    printf("deque new size: %u:\n", deq.size);
    free(item);
  }

  {
    deq_entry_s *item;
    deq_pop_tail(&deq, &item);
    mac_node_s *node = item->data;
    uint8_t *m = node->mac;
    printf("DEQ_POP_TAIL popped entry: %02X:%02X:%02X:%02X:%02X:%02X\n",
           m[0], m[1], m[2], m[3], m[4], m[5]);
    printf("deque new size: %u:\n", deq.size);
    free(item);
  }

  // for each deque entry
  {
    printf("Listing deque entries (%u):\n", deq.size);
    cnt = 0;
    deq_entry_s *item;
    DEQ_FOR_EACH(deq, item, list) {
      uint8_t *m = ((mac_node_s *)(item->data))->mac;
      struct in_addr *ip = &(((mac_node_s *)(item->data))->ip);
      printf("%u %02X:%02X:%02X:%02X:%02X:%02X %s\n",
             cnt++,
             m[0], m[1], m[2], m[3], m[4], m[5],
             inet_ntoa(*ip));
    }
  }

  printf("Listing hashtable entries:\n");
  cnt = cnt_init;
  hash_for_each_bits(tbl, hash_bits, bkt, cur, node) {
    cnt--;
    uint32_t key_calc = hash_time33((const char *)cur->mac, IFHWADDRLEN);
    uint32_t bkt_calc = calc_bkt(key_calc, 1 << hash_bits);
    if (cnt % printer == 0) {
      uint8_t *m = cur->mac;
      struct in_addr *ip = &cur->ip;
      printf("lst: %02X:%02X:%02X:%02X:%02X:%02X %s ",
             m[0], m[1], m[2], m[3], m[4], m[5], inet_ntoa(*ip));
      if (bkt != bkt_calc) printf("warning: bkt != bkt_calc bkt: %u bkt_calc: %u\n", bkt, bkt_calc);
      printf("bkt: %u k: %u\n", bkt, key_calc);
    }
  }

  {
    // test delete from the middle
    cnt = cnt_init;
    printf("%d\n", __LINE__);
    hash_for_each_safe_bits(tbl, hash_bits, bkt, tmp, cur, node) {
      if (cnt-- == cnt_init / 2) {
        printf("%d\n", __LINE__);
        uint8_t *m = cur->mac;
        printf("test deletion mac: %02X:%02X:%02X:%02X:%02X:%02X\n", m[0], m[1], m[2], m[3], m[4], m[5]);

        mac_node_s *found = get_by_mac_first_found(tbl, hash_bits, m);
        if (found) {
          printf("get by mac found: %u ip: %s\n", key, inet_ntoa(found->ip));
          hash_del(&found->node);
          free(found);
        }
        break;
      }
    }
  }
  // remove ht entries and ht
  mactable_free(tbl, bits);

  // remove deque entries
  deq_free(&deq);
  printf("check DEQ_IS_EMPTY: %d\n", deq_isempty(&deq));

  // deque itself is on stack no need to free.

  return 0;
}

// this is example hashtable container
typedef struct string_entry {
  struct hlist_node node; // hashtable list node structure
  char str[20];           // str ptr
} string_entry_t;

static void add_string_to_hashtable(hashtable_t *ht, const char *str) {
  string_entry_t *entry = malloc(sizeof(string_entry_t));
  size_t str_len = strlen(str);
  memcpy(entry->str, str, str_len + 1); // dup str
  uint32_t key = hash_time33(str, str_len);
  hashtable_add(ht, &entry->node, key);
}

static char *find_string_in_hashtable(hashtable_t *ht, const char *str) {
  uint32_t key = hash_time33(str, strlen(str));
  uint32_t bkt = calc_bkt(key, 1 << ht->bits);
  string_entry_t *entry;
  hlist_for_each_entry(entry, &ht->table[bkt], node) {
    if (strcmp(entry->str, str) == 0) {
      return entry->str;
    }
  }
  return NULL; // not found
}

static int delete_string_from_hashtable(hashtable_t *ht, const char *str) {
  uint32_t key = hash_time33(str, strlen(str));
  uint32_t bkt = calc_bkt(key, 1 << ht->bits);
  struct hlist_node *tmp;
  string_entry_t *cur;

  hlist_for_each_entry_safe(cur, tmp, &ht->table[bkt], node) {
    if (strcmp(cur->str, str) == 0) {
      hlist_del(&cur->node);
      free(cur);
      return 0; // deleted
    }
  }
  return 1; // not found
}

static size_t count_hashtable_collisions(hashtable_t *ht) {
  unsigned int collisions = 0;
  size_t size = (1 << ht->bits);
  for (uint32_t bkt = 0; bkt < size; ++bkt) {
    struct hlist_head *head = &ht->table[bkt];
    struct hlist_node *node;
    int bucket_count = 0;

    hlist_for_each(node, head) {
      ++bucket_count;
    }

    if (bucket_count > 1) {
      collisions += (bucket_count - 1); // only additional list entries counts as a collisions
    }
  }
  return collisions;
}

static size_t count_hashtable_entries(hashtable_t *ht) {
  size_t entries = 0;
  size_t size = (1 << ht->bits);
  for (uint32_t bkt = 0; bkt < size; ++bkt) {
    struct hlist_head *head = &ht->table[bkt];
    struct hlist_node *node;
    int bucket_count = 0;

    hlist_for_each(node, head) {
      ++bucket_count;
    }

    entries += bucket_count; // count each entry
  }
  return entries;
}

static void free_string_entry(void *entry) {
  free(entry);
}

static int do_hashtable_stuff2(uint8_t bits, float density, float print_freq_density) {
  printf("%s\n", __func__);
  
  hashtable_t *ht = create_hashtable(bits, free_string_entry);
  if (!ht) {
    perror("Failed to create hashtable");
    return 1;
  }

  // create random strings
  for (int i = 0; i < (1 << bits) / 2; i++) {
    char random_str[20];
    sprintf(random_str, "string_%d", rand());
    add_string_to_hashtable(ht, random_str);
  }

  // add test str
  add_string_to_hashtable(ht, "test_string_1");
  add_string_to_hashtable(ht, "test_string_2");

  // print test str
  char *found_str = find_string_in_hashtable(ht, "test_string_1");
  if (found_str) printf("Found: %s\n", found_str);

  found_str = find_string_in_hashtable(ht, "test_string_2");
  if (found_str) printf("Found: %s\n", found_str);

  printf("collisions: %zu\n", count_hashtable_collisions(ht));
  printf("entries:    %zu\n", count_hashtable_entries(ht));

  printf("deleting: %s\n", "test_string_1");
  int ret = 1;
  ret = delete_string_from_hashtable(ht, "test_string_1");
  printf("delete result: %s\n", ret ? "failed" : "ok");

  found_str = find_string_in_hashtable(ht, "test_string_1");
  if (!found_str) printf("NOT Found: %s as expected\n", "test_string_1");

  FREE_HASHTABLE(ht, string_entry_t, node, free_string_entry);

  return 0;
}

// more complicated example with dynamically allocated strings in a hash table
typedef struct string_entry2 {
  struct hlist_node node; // hashtable list node structure
  char *str;              // str ptr
} string_entry2_t;

void free_string_entry2(void *entry) {
  string_entry2_t *string_entry = (string_entry2_t *)entry;
  free(string_entry->str); // Free the dynamically allocated string
  free(string_entry);      // Free the structure itself
}

static void add_string_to_hashtable2(hashtable_t *ht, const char *str) {
  string_entry2_t *entry = malloc(sizeof(string_entry2_t));
  size_t str_len = strlen(str);
  entry->str = malloc(str_len + 1);
  memcpy(entry->str, str, str_len + 1); // dup str
  uint32_t key = hash_time33(str, str_len);
  hashtable_add(ht, &entry->node, key);
}

static int delete_string_from_hashtable2(hashtable_t *ht, const char *str) {
  // uint32_t key = hash_time33(str, strlen(str));
  uint32_t key = hash_time33(str, strlen(str));
  uint32_t bkt = calc_bkt(key, 1 << ht->bits);
  struct hlist_node *tmp;
  string_entry2_t *cur;

  hlist_for_each_entry_safe(cur, tmp, &ht->table[bkt], node) {
    if (strcmp(cur->str, str) == 0) {
      hlist_del(&cur->node);
      free_string_entry2(cur);
      return 0; // deleted
    }
  }
  return 1; // not found
}

static int do_hashtable_stuff3(uint8_t bits, float density, float print_freq_density) {
  printf("%s\n", __func__);
  int max;
  
  hashtable_t *ht = create_hashtable(bits, free_string_entry2);
  if (!ht) {
    perror("Failed to create hashtable");
    return 1;
  }

  // create random strings
  max = (1 << bits) * density;
  for (int i = 0; i < max; i++) {
    char random_str[20];
    sprintf(random_str, "string_%d", rand());
    add_string_to_hashtable2(ht, random_str);
  }

  // add test str
  add_string_to_hashtable2(ht, "test_string_1");
  add_string_to_hashtable2(ht, "test_string_2");

  // list entries
  uint32_t bkt = 0;
  string_entry2_t *cur;
  uint32_t cnt = 0;
  int print_each = max * print_freq_density;
  hash_for_each_bits(ht->table, ht->bits, bkt, cur, node) {
    if(cnt % print_each == 0) printf("cnt: %u bkt: %u string: %s\n", cnt, bkt, cur->str);
    cnt++;
  }

  // print test str
  char *found_str = find_string_in_hashtable(ht, "test_string_1");
  if (found_str) printf("Found: %s\n", found_str);

  found_str = find_string_in_hashtable(ht, "test_string_2");
  if (found_str) printf("Found: %s\n", found_str);

  size_t ht_size = 1 << ht->bits;
  size_t ht_col = count_hashtable_collisions(ht);
  size_t ht_entries = count_hashtable_entries(ht);
  float col_div_entries = (float)ht_col / ht_entries * 100;
  printf("              size: %zu\n", ht_size);
  printf("        collisions: %zu\n", ht_col);
  printf("           entries: %zu\n", ht_entries);
  printf("collisions/entries: %.02f%%\n", col_div_entries);

  printf("deleting: %s\n", "test_string_1");
  int ret = 1;
  ret = delete_string_from_hashtable2(ht, "test_string_1");
  printf("delete result: %s\n", ret ? "failed" : "ok");

  found_str = find_string_in_hashtable(ht, "test_string_1");
  if (!found_str) printf("NOT Found: %s as expected\n", "test_string_1");

  FREE_HASHTABLE(ht, string_entry_t, node, free_string_entry2);

  return 0;
}

int main(int argc, char *argv[]) {
  int bits = 18;
  float density = 0.5;
  float print_freq_density = 0.5;
  /* cli arguments parse */
  argv0 = *argv; /* set program name */
  // if (argc == 1) usage();

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

  do_hashtable_stuff(bits, density, print_freq_density);
  do_hashtable_stuff2(bits, density, print_freq_density);
  do_hashtable_stuff3(bits, density, print_freq_density);
  return 0;
}