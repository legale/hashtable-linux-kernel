#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>


#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif


#ifdef JEMALLOC
#include "jemalloc.h"
#endif
#include "assoc_array.h"


#ifdef LEAKCHECK
#include "leak_detector_c.h"
#endif

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

static char *diff_timespec(struct timespec t1, struct timespec t2) {
  // Проверка на корректность: t2 > t1
  if (t2.tv_sec < t1.tv_sec || (t2.tv_sec == t1.tv_sec && t2.tv_nsec < t1.tv_nsec)) {
    return 0;
  }

  int64_t sec_diff = t2.tv_sec - t1.tv_sec;
  int64_t nsec_diff = t2.tv_nsec - t1.tv_nsec;

  // Если наносекундная разница отрицательная, вычитаем 1 секунду
  if (nsec_diff < 0) {
    sec_diff -= 1;
    nsec_diff += 1000000000; // Добавляем 1 миллиард наносекунд (1 секунда)
  }

  // Общая разница в наносекундах
  int64_t total_nsecs = sec_diff * 1000000000 + nsec_diff;

  char *retstr = malloc(128);
  // Определяем наиболее подходящие единицы измерения
  if (total_nsecs >= 1000000000) { // Больше или равно 1 секунде
    snprintf(retstr, 256, "%f %s", total_nsecs / 1000000000.0f, "s");
  } else if (total_nsecs >= 1000000) { // Меньше секунды, но больше или равно 1 миллисекунде
    snprintf(retstr, 256, "%f %s", total_nsecs / 1000000.0f, "ms");
  } else if (total_nsecs >= 1000) { // Меньше миллисекунды, но больше или равно 1 микросекунде
    snprintf(retstr, 256, "%f %s", total_nsecs / 1000.0f, "µs");
  } else { // Меньше микросекунды
    snprintf(retstr, 256, "%" PRId64 " %s", total_nsecs, "ns");
  }
  return retstr;
}

static void usage(void) {
  fprintf(stdout,
          "Usage: %s [-h] [BITS_SHIFT] [DENSITY] [PRINT_FREQ_DENSITY]\n"
          "Options:\n"
          "  -h                        Show this help message and exit\n"
          "     BITS_SHIFT             Specify the bit shift for array size, default is 18\n"
          "     DENSITY                Set the density for initial fill, default is 0.5\n"
          "     PRINT_FREQ_DENSITY     Set the frequency density for printing, default is 0.5\n"
          "\n"
          "Example: %s -b 10 -d 0.5 -p 0.1\n"
          "         This will create an array with size determined by a bit shift of 10,\n"
          "         fill it to 50%% density, and print with a frequency of 10%%.\n"
          "\n",
          argv0, argv0);
  exit(0);
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

typedef struct mac_node {
  struct in_addr ip;
  uint8_t mac[ETH_ALEN];
  const char *hostname;
} mac_node_t;

static void generate_mac(uint8_t *mac) {
  for (int i = 0; i < ETH_ALEN; i++) {
    mac[i] = random() % 256;
  }
}

static void generate_ipv4(struct in_addr *ip) {
  ip->s_addr = random() % INT32_MAX;
}

static void generate_hostname(const char **hostname, int index) {
  char buffer[50];                                      // Buffer for hostname
  snprintf(buffer, sizeof(buffer), "Device %d", index); // Generate a hostname
  *hostname = strdup(buffer);                           // Duplicate the hostname string and assign to the pointer
}

void print_mac_node(const mac_node_t *node) {
  // Convert MAC address to string format
  char mac_str[18]; // 17 characters for MAC address + 1 for null terminator
  snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
           node->mac[0], node->mac[1], node->mac[2],
           node->mac[3], node->mac[4], node->mac[5]);

  // Convert IP address to string format
  char ip_str[INET_ADDRSTRLEN]; // Buffer for the IPv4 address
  inet_ntop(AF_INET, &(node->ip), ip_str, sizeof(ip_str));

  printf("MAC=%s, IP=%s, Hostname=%s\n", mac_str, ip_str, node->hostname);
}

void free_entry(void *entry) {
  assoc_array_entry_t *e = (assoc_array_entry_t *)entry;
  mac_node_t *mac_node = (mac_node_t *)e->data;

  if (mac_node->hostname) {
    free((void *)mac_node->hostname);
  }

  free(mac_node);

  // no need to free key ptr, because it points to mac_node->mac

  free(e);
}

int fill_entry(assoc_array_entry_t *entry, void *data, void *key, uint8_t key_size) {

  // Key points directly to the mac address within the mac_node structure
  entry->key = key;
  entry->key_size = key_size;
  entry->data = data;
  return 0;
}

int run_example_code(int bits, float density, float print_freq_density);

int main(int argc, char *argv[]) {
  int bits = 18;
  float density = 0.5;
  float print_freq_density = 0.5;
  argv0 = *argv; // Set program name for usage output

  // Parse command line arguments
  
	while (argc > 1) {
    NEXT_ARG();
    if (matches(*argv, "-h") || matches(*argv, "--help")) {
      usage();
    }
		bits = atoi(*argv);
    if (NEXT_ARG_OK()) {
      NEXT_ARG();
      density = atof(*argv);
    }
    if (NEXT_ARG_OK()) {
      NEXT_ARG();
      print_freq_density = atof(*argv);
    }
  }

  printf("Configuration: bits=%d, density=%.2f, print_freq_density=%.2f\n", bits, density, print_freq_density);

  return run_example_code(bits, density, print_freq_density);
}

int run_example_code(int bits, float density, float print_freq_density) {
  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);


  // Initialize the associative array
  assoc_array_t *arr = array_create(bits, free_entry, fill_entry);
  mac_node_t *node;
  assoc_array_entry_t *e;
  int print_divisor = 1;
  if (print_freq_density > 0) {
    print_divisor = 1.0 / print_freq_density;
  }
  printf("print_devisor: %d\n", print_divisor);

  // Calculate the number of entries to add based on the specified density
  int entries_to_add = (1 << bits) * density;
  for (int i = 0; i < entries_to_add; i++) {
    node = malloc(sizeof(mac_node_t)); // Allocate memory for the node
    if (!node) {
      // Handle memory allocation failure if needed
      continue;
    }
    generate_mac(node->mac);               // Fill the MAC address
    generate_ipv4(&node->ip);              // Fill the IP address
    generate_hostname(&node->hostname, i); // Fill the hostname

    // Add the node to the associative array using its MAC address as the key
    array_add_replace(arr, node, node->mac, ETH_ALEN);
    if ((i % print_divisor) == 0) {
      printf("entry %d:\n", i);
      print_mac_node(node);
    }
  }

  // Retrieve and delete the last entry as an example
  assoc_array_entry_t *last_entry = array_get_last(arr);
  node = (mac_node_t *)last_entry->data;
  printf("last entry array_get_last\n");
  print_mac_node(node);

  if (last_entry) {
    // Perform operations with last_entry if needed
    array_del_first(arr);
  }

  // Example of adding a new entry that might replace an existing one
  node = malloc(sizeof(mac_node_t));
  generate_mac(node->mac);
  generate_ipv4(&node->ip);
  generate_hostname(&node->hostname, 100);
  printf("new entry array_add_replace\n");
  print_mac_node(node);

  array_add_replace(arr, node, node->mac, ETH_ALEN);
  e = array_get_by_key(arr, node->mac, ETH_ALEN);
  if (e) {
    node = (mac_node_t *)e->data;
    printf("array_get_by_key last added entry:\n");
    print_mac_node(node);
  }

  e = array_get_first(arr);
  if (e) {
    node = (mac_node_t *)e->data;
    printf("array_get_first:\n");
    print_mac_node(node);
  }

  // Clean up: Free the associative array and its contents
  int coll = array_collision_percent(arr);
  array_free(arr);

#ifdef LEAKCHECK
	//this is memleak report produced by leak_detector_c.c
	report_mem_leak();
#endif

  // Get the start time
  clock_gettime(CLOCK_MONOTONIC, &end);

  char *t = diff_timespec(start, end);
  printf("time passed: %s collisions: %d%%\n", t, coll);
  free(t);

  return 0;
}
