#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>

#include "assoc_array.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

#define FULL_FILL 512
#define HALF_FILL 256

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

typedef struct mac_node {
    struct in_addr ip;
    uint8_t mac[ETH_ALEN];
    const char *hostname; 
} mac_node_t;


void free_entry(void *entry) {
    assoc_array_entry_t *e = (assoc_array_entry_t *)entry;
    mac_node_t *mac_node = (mac_node_t *)e->data; 

    if (mac_node->hostname) {
        free((void*)mac_node->hostname);
    }

    free(mac_node);

		//no need to free key ptr, because it points to mac_node->mac

    free(e);
}

int fill_entry(assoc_array_entry_t *entry, void *data, void *key, uint8_t key_size) {

    // Key points directly to the mac address within the mac_node structure
    entry->key = key;
    entry->key_size = key_size;
    entry->data = data;
		return 0;
}

// The functions array_create, array_add, array_get_last, array_del_last, and array_free need to be implemented based on your associative array structure.

void test_mac_node_operations() {
    // Assuming assoc_array_t and related functions are defined elsewhere in your project
    assoc_array_t *arr = array_create(ilog2(FULL_FILL), free_entry, fill_entry); // 2^9 = 512
		TEST_ASSERT_NOT_NULL(arr);

    // Filling the array up to half with random data
    for (int i = 0; i < HALF_FILL; ++i) {
        mac_node_t *node = (mac_node_t *)malloc(sizeof(mac_node_t));
        if (!node) {
            printf("Failed to allocate memory for mac_node_t\n");
            array_free(arr);
            return;
        }
        node->ip.s_addr = rand(); // Example IP address
        for (int j = 0; j < ETH_ALEN; ++j) {
            node->mac[j] = rand() % 256; // Example MAC address
        }  
				char hostname[30];
        snprintf(hostname, sizeof(hostname), "hostname%d", i);
        node->hostname = strdup(hostname); // Creating a unique hostname

        // Adding the node to the associative array using MAC address as the key
        int ret = array_add_replace(arr, node, node->mac, ETH_ALEN);
				TEST_ASSERT_EQUAL_INT(0, ret);

				//check size
				TEST_ASSERT_EQUAL_UINT32(i + 1, arr->size);
    }

    // Getting the last entry and then adding a new one
    assoc_array_entry_t *last_entry = array_get_last(arr);
    if (last_entry == NULL) {
        printf("Failed to get the last entry.\n");
        array_free(arr);
        return;
    }

    // Adding a new mac_node entry
    mac_node_t *new_node = (mac_node_t *)malloc(sizeof(mac_node_t));
    if (!new_node) {
        printf("Failed to allocate memory for the new mac_node_t\n");
        array_free(arr);
        return;
    }
    new_node->ip.s_addr = rand(); // Example IP address for the new node
    for (int j = 0; j < ETH_ALEN; ++j) {
        new_node->mac[j] = rand() % 256; // Example MAC address for the new node
    }
    new_node->hostname = strdup("new hostname"); // Example hostname for the new node

    array_add(arr, new_node, new_node->mac, ETH_ALEN);
    array_del_last(arr); // Deleting the last entry

    // Verifying the array size is HALF_FILL after deletion
    if (arr->size != HALF_FILL) {
        printf("The array size is not as expected after deletion.\n");
    }

    array_free(arr); // Freeing the array and its contents
}


int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_mac_node_operations);	

  return UNITY_END();
}