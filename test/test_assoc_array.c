#include <string.h>

#include "assoc_array.h"
#include "unity.h"

#include "mock_mem_functions.h" //this header allow to use set_mem_functions() to redefine original

static assoc_array_t *arr = NULL;
static void *data = NULL;
static void *data2 = NULL;
static void *data3 = NULL;
static char key[] = "test_key";
static char key2[] = "another_test_key";

void *mock_malloc(size_t size) {
  return NULL; // Simulate memory allocation failure
}

hashtable_t *mock_ht_create(uint32_t bits) {
  return NULL; // Simulate memory allocation failure
}

void setUp(void) {}
void tearDown(void) {}

void free_entry(void *entry) {
  assoc_array_entry_t *assoc_entry = (assoc_array_entry_t *)entry;
  // printf("free assoc_entry->data\n");
  free(assoc_entry->data); // Free the dynamically allocated data
  // printf("free assoc_entry->key\n");
  free(assoc_entry->key); // Free the dynamically allocated key
  // printf("free assoc_entry\n");
  free(assoc_entry); // Free the entry itself
}

void test_array_create_failure() {
  assoc_array_t *arr_failed = NULL;
  set_memory_functions(mock_malloc, calloc, realloc, free);
  arr_failed = array_create(10, free_entry, NULL);
  TEST_ASSERT_NULL(arr_failed);
  set_memory_functions(malloc, calloc, realloc, free);

  // test ht_create failed
  set_ht_create(mock_ht_create);
  arr_failed = array_create(10, free_entry, NULL);
  TEST_ASSERT_NULL(arr_failed);
  set_ht_create(ht_create); // restore original ht_create function
}

void test_array_create_default(void) {
  uint32_t bits = 10;
  arr = array_create(bits, NULL, NULL);

  TEST_ASSERT_NOT_NULL(arr); // Check if the array was successfully created

  if (arr != NULL) {
    TEST_ASSERT_NOT_NULL(arr->ht->table);          // Ensure the hash table exists
    TEST_ASSERT_EQUAL_UINT32(bits, arr->ht->bits); // Verify the number of bits matches the expected value
    TEST_ASSERT_EQUAL_UINT32(0, arr->size);        // Check that the initial size is set to 0
  }
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
  arr = NULL;
}

void test_array_free_non_empty(void) {
  TEST_ASSERT_NOT_NULL(arr);
  TEST_ASSERT_NOT_EQUAL_UINT32(0, arr->size);

  int ret = array_free(arr);
  arr = NULL;
  TEST_ASSERT_EQUAL_INT(0, ret);
}

int mock_array_fill_func(assoc_array_entry_t *entry, void *data, void *key, uint8_t key_size) {
  return 1;
}

void test_array_create_add_failure_free(void) {
  // create array normally
  test_array_create();

  // replace fill_entry function with test mock
  arr->fill_entry = mock_array_fill_func;

  TEST_ASSERT_NOT_EQUAL(0, array_add(arr, NULL, NULL, 0));

  // free empty array
  test_array_free_empty();
}

void test_array_create_add_failure2_free(void) {
  // create array normally
  test_array_create();

  // replace malloc function
  set_memory_functions(mock_malloc, calloc, realloc, free);
  TEST_ASSERT_NOT_EQUAL(0, array_add(arr, NULL, NULL, 0));
  // restore malloc function
  set_memory_functions(malloc, calloc, realloc, free);

  // free empty array
  test_array_free_empty();
}

void test_array_add(void) {
  // key and data are global vars
  data = malloc(strlen("test_data") + 1); // +1 for null terminator
  strcpy(data, "test_data");
  uint8_t key_size = strlen(key) + 1; // + 1 for null terminator

  int add_result = array_add(arr, data, key, key_size);
  TEST_ASSERT_EQUAL_INT(0, add_result);
}

void test_array_add_with_null(void) {
  // key and data are global vars
  data = malloc(strlen("test_data") + 1); // +1 for null terminator
  strcpy(data, "test_data");
  uint8_t key_size = strlen(key) + 1; // + 1 for null terminator

  int add_result = array_add(NULL, data, key, key_size);
  TEST_ASSERT_EQUAL_INT(-1, add_result);
  free(data);
}

void test_array_get_by_key(void) {
  // key and data are global vars
  uint8_t key_size = strlen(key) + 1;
  // Retrieve the element by key
  assoc_array_entry_t *entry = array_get_by_key(arr, key, key_size);
  TEST_ASSERT_NOT_NULL(entry);                 // Ensure the element is found
  TEST_ASSERT_EQUAL_STRING(data, entry->data); // Compare the retrieved data with the original
}

void test_array_get_by_key_with_null(void) {
  // key and data are global vars
  uint8_t key_size = strlen(key) + 1;
  // Retrieve the element by key
  assoc_array_entry_t *entry = array_get_by_key(NULL, key, key_size);
  TEST_ASSERT_NULL(entry);
}

void test_array_del(void) {
  // key and data are global vars
  uint8_t key_size = strlen(key) + 1;
  // Retrieve the element by key
  int ret = array_del(arr, key, key_size);
  TEST_ASSERT_EQUAL_INT(0, ret);
  TEST_ASSERT_EQUAL_UINT32(0, arr->size);
}

void test_array_del_first(void) {
  int ret = array_del_first(arr);
  TEST_ASSERT_EQUAL_INT(0, ret);
  TEST_ASSERT_EQUAL_UINT32(0, arr->size);
}
void test_array_del_last(void) {
  int ret = array_del_last(arr);
  TEST_ASSERT_EQUAL_INT(0, ret);
  TEST_ASSERT_EQUAL_UINT32(0, arr->size);
}

void test_array_add_replace(void) {
  int ret;

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
}

void test_array_create_fill_half_capacity_del_free(void) {
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

void test_array_create_get_first_get_last_with_multiple_entries_free(void) {
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

  // cleanup
  test_array_free_non_empty();
}

int main(void) {
  UNITY_BEGIN();

  printf("TEST BLOCK: test array basics\n");
  RUN_TEST(test_array_create_failure);
  RUN_TEST(test_array_create_default);
  RUN_TEST(test_array_free_empty);
  RUN_TEST(test_array_create);
  RUN_TEST(test_array_free_empty);

  printf("TEST BLOCK: test array add failure\n");
  RUN_TEST(test_array_create_add_failure_free);
  RUN_TEST(test_array_create_add_failure2_free);
  

  printf("TEST BLOCK: test array with default fill_array function\n");
  RUN_TEST(test_array_create_default);
  RUN_TEST(test_array_add);
  RUN_TEST(test_array_get_by_key);
  RUN_TEST(test_array_add_with_null);
  RUN_TEST(test_array_get_by_key_with_null);
  RUN_TEST(test_array_free_non_empty);

  printf("TEST BLOCK: test array with custom fill_array function\n");
  RUN_TEST(test_array_create);
  RUN_TEST(test_array_add);
  RUN_TEST(test_array_get_by_key);
  RUN_TEST(test_array_add_with_null);
  RUN_TEST(test_array_get_by_key_with_null);
  RUN_TEST(test_array_free_non_empty);

  printf("TEST BLOCK: test array create, add, del, free\n");
  RUN_TEST(test_array_create);
  RUN_TEST(test_array_add);
  RUN_TEST(test_array_del);
  RUN_TEST(test_array_free_empty);

  printf("TEST BLOCK: test array create, add_replace, free\n");
  RUN_TEST(test_array_create);
  RUN_TEST(test_array_add_replace);
  RUN_TEST(test_array_free_non_empty);

  printf("TEST BLOCK: test array create, add, del_first, del_last, free\n");
  RUN_TEST(test_array_create);
  RUN_TEST(test_array_add);
  RUN_TEST(test_array_del_first);
  RUN_TEST(test_array_add);
  RUN_TEST(test_array_del_last);
  RUN_TEST(test_array_free_empty);

  RUN_TEST(test_array_create_fill_half_capacity_del_free);
  RUN_TEST(test_array_create_get_first_get_last_with_multiple_entries_free);

  return UNITY_END();
}
