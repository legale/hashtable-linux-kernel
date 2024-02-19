#include <string.h>

#include "assoc_array.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static assoc_array_t *arr = NULL;
static void *data = NULL;
static void *data2 = NULL;
static void *data3 = NULL;
static char key[] = "test_key";
static char key2[] = "another_test_key";

void free_entry(void *entry) {
  assoc_array_entry_t *assoc_entry = (assoc_array_entry_t *)entry;
  // printf("free assoc_entry->data\n");
  free(assoc_entry->data); // Free the dynamically allocated data
  // printf("free assoc_entry->key\n");
  free(assoc_entry->key); // Free the dynamically allocated key
  // printf("free assoc_entry\n");
  free(assoc_entry); // Free the entry itself
}

void test_array_create(void) {
  uint32_t bits = 10;
  arr = array_create(bits, free_entry, NULL);

  TEST_ASSERT_NOT_NULL(arr); // Check if the array was successfully created

  if (arr != NULL) {
    TEST_ASSERT_NOT_NULL(arr->ht->table);               // Ensure the hash table exists
    TEST_ASSERT_EQUAL_UINT32(bits, arr->ht->bits);      // Verify the number of bits matches the expected value
    TEST_ASSERT_EQUAL_PTR(free_entry, arr->free_entry); // Confirm the free_entry callback is correctly set
    TEST_ASSERT_EQUAL_UINT32(0, arr->size);             // Check that the initial size is set to 0
  }
}

void test_array_free_empty(void) {
  TEST_ASSERT_NOT_NULL(arr);
  TEST_ASSERT_EQUAL_UINT32(0, arr->size);

  int ret = array_free(arr);
  TEST_ASSERT_EQUAL_INT(0, ret);
}

void test_array_free_non_empty(void) {
  TEST_ASSERT_NOT_NULL(arr);
  TEST_ASSERT_NOT_EQUAL_UINT32(0, arr->size);

  int ret = array_free(arr);
  TEST_ASSERT_EQUAL_INT(0, ret);
}

void test_array_add(void) {
  test_array_create();
  // key and data are global vars
  data = malloc(strlen("test_data") + 1); // +1 for null terminator
  strcpy(data, "test_data");
  uint8_t key_size = strlen(key) + 1; // + 1 for null terminator

  int add_result = array_add(arr, data, key, key_size);
  TEST_ASSERT_EQUAL_INT(0, add_result);
}

void test_array_get_by_key(void) {
  // key and data are global vars
  uint8_t key_size = strlen(key) + 1;
  // Retrieve the element by key
  assoc_array_entry_t *entry = array_get_by_key(arr, key, key_size);
  TEST_ASSERT_NOT_NULL(entry);                 // Ensure the element is found
  TEST_ASSERT_EQUAL_STRING(data, entry->data); // Compare the retrieved data with the original
}

void test_array_del(void) {
  test_array_add();
  // key and data are global vars
  uint8_t key_size = strlen(key) + 1;
  // Retrieve the element by key
  int ret = array_del(arr, key, key_size);
  TEST_ASSERT_EQUAL_INT(0, ret);
  TEST_ASSERT_EQUAL_UINT32(0, arr->size);
  test_array_free_empty();
}

void test_array_add_replace(void) {
  int ret;
  arr = array_create(10, free_entry, NULL);
  TEST_ASSERT_NOT_NULL(arr);

  data = malloc(strlen("initial_data") + 1); // +1 for null terminator
  strcpy(data, "initial_data");
  uint8_t key_size = strlen(key) + 1; // + 1 for null terminator

  data2 = malloc(strlen("initial_data2") + 1); // +1 for null terminator
  strcpy(data2, "initial_data2");
  uint8_t key_size2 = strlen(key2) + 1; // + 1 for null terminator

  ret = array_add_replace(arr, data, key, key_size);
  TEST_ASSERT_EQUAL_INT(0, ret);

  ret = array_add_replace(arr, data2, key2, key_size2);
  TEST_ASSERT_EQUAL_INT(0, ret);

  // check size
  TEST_ASSERT_EQUAL_UINT32(2, arr->size);

  assoc_array_entry_t *entry = array_get_by_key(arr, key, key_size);
  TEST_ASSERT_NOT_NULL(entry);
  TEST_ASSERT_EQUAL_STRING(data, entry->data);

  data3 = malloc(strlen("new_data") + 1); // Allocate new data
  strcpy(data3, "new_data");
  ret = array_add_replace(arr, data3, key, key_size);
  TEST_ASSERT_EQUAL_INT(0, ret);

  assoc_array_entry_t *replaced_entry = array_get_by_key(arr, key, key_size);
  TEST_ASSERT_NOT_NULL(replaced_entry);
  TEST_ASSERT_EQUAL_STRING(data3, replaced_entry->data);
  TEST_ASSERT_EQUAL_UINT32(2, arr->size);
  test_array_free_non_empty();
}

void test_array_fill_and_half_delete(void) {
  uint32_t bits = 10; // Create an array with a specific capacity
  arr = array_create(bits, free_entry, NULL);
  TEST_ASSERT_NOT_NULL(arr);

  int capacity = 1 << bits;         // Calculate the actual capacity based on bits
  int half_capacity = capacity / 2; // Calculate half of the capacity for the test

  // Fill half of the array's capacity with unique entries
  for (int i = 0; i < half_capacity; ++i) {
    char dynamic_key[20];
    sprintf(dynamic_key, "key_%d", i); // Create a unique key for each entry
    char *dynamic_data = malloc(strlen("data") + 10);
    sprintf(dynamic_data, "data_%d", i); // Create unique data for each entry

    uint8_t key_size = strlen(dynamic_key) + 1; // Include the null terminator
    int add_result = array_add(arr, dynamic_data, dynamic_key, key_size);
    TEST_ASSERT_EQUAL_INT(0, add_result);
  }

  // Check the size after filling
  TEST_ASSERT_EQUAL_UINT32(half_capacity, arr->size);

  // Delete half of the added entries
  for (int i = 0; i < half_capacity; i += 2) { // Delete every other entry
    char dynamic_key[20];
    sprintf(dynamic_key, "key_%d", i);
    uint8_t key_size = strlen(dynamic_key) + 1; // Include the null terminator
    int ret = array_del(arr, dynamic_key, key_size);
    TEST_ASSERT_EQUAL_INT(0, ret); // Ensure each deletion is successful
  }

  // Check the size after deletion
  TEST_ASSERT_EQUAL_UINT32(half_capacity / 2, arr->size);

  // Verify deletion was successful by attempting to delete again
  for (int i = 0; i < half_capacity; i += 2) {
    char dynamic_key[20];
    sprintf(dynamic_key, "key_%d", i);
    uint8_t key_size = strlen(dynamic_key) + 1; // Include the null terminator
    int ret = array_del(arr, dynamic_key, key_size);
    TEST_ASSERT_EQUAL_INT(1, ret);                                             // Expect failure since it should already be deleted
    assoc_array_entry_t *entry = array_get_by_key(arr, dynamic_key, key_size); // another check with array_get_by_key
    TEST_ASSERT_NULL(entry);                                                   // Expect failure since it should already be deleted
  }

  // Cleanup
  test_array_free_non_empty();
}

void test_array_get_first_get_last_with_multiple_entries(void) {
  // Create a new associative array
  arr = array_create(10, free_entry, NULL);
  TEST_ASSERT_NOT_NULL(arr);

  char key[30];                // Buffer for the key
  char data_prefix[] = "data"; // Prefix for data values
  char *data_entries[20];      // Array to store pointers to data strings
  uint8_t key_size;

  // Add 20 elements to the array
  for (int i = 0; i < 20; ++i) {
    snprintf(key, sizeof(key), "key%d", i); // Generating the key
    key_size = strlen(key) + 1;             // Calculating the key size including the null terminator

    // Allocating memory for data and generating data string
    data_entries[i] = malloc(strlen(data_prefix) + 10);
    snprintf(data_entries[i], strlen(data_prefix) + 10, "%s%d", data_prefix, i);

    // Adding the element to the array
    array_add(arr, data_entries[i], key, key_size);
  }

  // Retrieve the first element and verify it
  assoc_array_entry_t *first_entry = array_get_first(arr);
  TEST_ASSERT_NOT_NULL(first_entry);
  assoc_array_entry_t *last_entry = array_get_last(arr);
  TEST_ASSERT_NOT_NULL(last_entry);
  // Verifying that the data of the first element matches the expected
  TEST_ASSERT_EQUAL_STRING(data_entries[0], first_entry->data);
  TEST_ASSERT_EQUAL_STRING(data_entries[19], last_entry->data);

  array_free(arr);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_array_create);
  RUN_TEST(test_array_free_empty);

  RUN_TEST(test_array_add);
  RUN_TEST(test_array_get_by_key);
  RUN_TEST(test_array_free_non_empty);

  RUN_TEST(test_array_del);
  RUN_TEST(test_array_add_replace);
  RUN_TEST(test_array_fill_and_half_delete);
  RUN_TEST(test_array_get_first_get_last_with_multiple_entries);
	

  return UNITY_END();
}
