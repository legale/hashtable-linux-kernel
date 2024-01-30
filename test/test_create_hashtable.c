#include <string.h>

#include "hashtable.h"
#include "unity.h"

void setUp(void) {}

void tearDown(void) {}

typedef struct string_entry {
  struct hlist_node node; // hashtable list node structure
  char *str;              // str ptr
} string_entry_t;

void free_entry(void *entry) {
  string_entry_t *string_entry = (string_entry_t *)entry;
  free(string_entry->str); // Free the dynamically allocated string
  free(string_entry);      // Free the structure itself
}

static void add_string_to_hashtable(hashtable_t *ht, const char *str) {
  string_entry_t *entry = malloc(sizeof(string_entry_t));
  size_t str_len = strlen(str);
  entry->str = malloc(str_len + 1);
  memcpy(entry->str, str, str_len + 1); // dup str
  uint32_t key = hash_time33(str, str_len);
  hashtable_add(ht, &entry->node, key);
}

static int delete_string_from_hashtable(hashtable_t *ht, const char *str) {
  // uint32_t key = hash_time33(str, strlen(str));
  uint32_t key = hash_time33(str, strlen(str));
  uint32_t bkt = calc_bkt(key, 1 << ht->bits);
  struct hlist_node *tmp;
  string_entry_t *cur;

  hlist_for_each_entry_safe(cur, tmp, &ht->table[bkt], node) {
    if (strcmp(cur->str, str) == 0) {
      hlist_del(&cur->node);
      free_entry(cur);
      return 0; // deleted
    }
  }
  return 1; // not found
}

static char **find_string_in_hashtable(hashtable_t *ht, const char *str) {
  uint32_t key = hash_time33(str, strlen(str));
  uint32_t bkt = calc_bkt(key, 1 << ht->bits);
  string_entry_t *entry;
  hlist_for_each_entry(entry, &ht->table[bkt], node) {
    if (strcmp(entry->str, str) == 0) {
      return &entry->str;
    }
  }
  return NULL; // not found
}

void test_create_hashtable(void) {
  uint32_t bits = 12;

  hashtable_t *ht = create_hashtable(bits, free_entry);
  // check creation
  TEST_ASSERT_NOT_NULL(ht);

  if (ht != NULL) {
    TEST_ASSERT_EQUAL_UINT32(bits, ht->bits);
    TEST_ASSERT_NOT_NULL(ht->table);
    TEST_ASSERT_NOT_NULL(ht->free_entry);

    // free ht
    FREE_HASHTABLE(ht, string_entry_t, node, free_entry);
  }

  // check if there is no callback passed
  hashtable_t *ht_null = create_hashtable(bits, NULL);
  TEST_ASSERT_NULL(ht_null);
}

#define TEST_STRING "just_test_string"

void test_add_to_ht(void) {
  hashtable_t *ht = create_hashtable(10, free_entry);
  TEST_ASSERT_NOT_NULL(ht);

  string_entry_t *entry = malloc(sizeof(string_entry_t));
  entry->str = malloc(sizeof(TEST_STRING) + 1);
  memcpy(entry->str, TEST_STRING, sizeof(TEST_STRING) + 1);
  uint32_t key = hash_time33(entry->str, sizeof(TEST_STRING));
  hashtable_add(ht, &entry->node, key);

  char **found_str = find_string_in_hashtable(ht, TEST_STRING);
  TEST_ASSERT_EQUAL_PTR(entry->str, *found_str);

  string_entry_t *found_entry = container_of(found_str, string_entry_t, str);
  TEST_ASSERT_EQUAL_PTR(entry, found_entry);

  // this should free everything in the hashtable
  FREE_HASHTABLE(ht, string_entry_t, node, free_entry);
}

void test_delete_string_from_hashtable(void) {
  char **found_str;

  hashtable_t *ht = create_hashtable(10, free_entry);
  TEST_ASSERT_NOT_NULL(ht);

  string_entry_t *entry = malloc(sizeof(string_entry_t));
  entry->str = malloc(sizeof(TEST_STRING) + 1);

  strcpy(entry->str, TEST_STRING);
  uint32_t key = hash_time33(entry->str, strlen(entry->str));
  hashtable_add(ht, &entry->node, key);

  found_str = find_string_in_hashtable(ht, TEST_STRING);
  TEST_ASSERT_EQUAL_PTR(entry->str, *found_str);

  int delete_result = delete_string_from_hashtable(ht, TEST_STRING);
  TEST_ASSERT_EQUAL_INT(0, delete_result);

  found_str = find_string_in_hashtable(ht, TEST_STRING);
  TEST_ASSERT_NULL(found_str);

  FREE_HASHTABLE(ht, string_entry_t, node, free_entry);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_create_hashtable);
  RUN_TEST(test_add_to_ht);
  RUN_TEST(test_delete_string_from_hashtable);
  return UNITY_END();
}
