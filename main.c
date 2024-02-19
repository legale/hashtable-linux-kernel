#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <string.h>

#include "assoc_array.h"

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
  uint8_t mac[IFHWADDRLEN];
  const char *hostname;
} mac_node_t;

static void generate_mac(uint8_t *mac) {
  for (int i = 0; i < IFHWADDRLEN; i++) {
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
    } else {
      NEXT_ARG();
      if (NEXT_ARG_OK()) bits = atoi(*argv);
      NEXT_ARG();
      if (NEXT_ARG_OK()) bits = density = atof(*argv);
      NEXT_ARG();
      if (NEXT_ARG_OK()) bits = print_freq_density = atof(*argv);
    }
  }

  printf("Configuration: bits=%d, density=%.2f, print_freq_density=%.2f\n", bits, density, print_freq_density);

  return run_example_code(bits, density, print_freq_density);
}

int run_example_code(int bits, float density, float print_freq_density) {
  // Initialize the associative array with the specified number of bits for indexing
  assoc_array_t *arr = array_create(bits, free_entry, fill_entry);
  mac_node_t *node;
  assoc_array_entry_t *e;

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
    array_add_replace(arr, node, node->mac, IFHWADDRLEN);
    if (i == 0){
		  printf("first added entry:\n");
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

  array_add_replace(arr, node, node->mac, IFHWADDRLEN);
  e = array_get_by_key(arr, node->mac, IFHWADDRLEN);
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
  array_free(arr);

  return 0;
}
