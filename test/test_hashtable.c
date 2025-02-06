#include <string.h>
#include <time.h>

#include "hashtable.h"
#include "mock_mem_functions.h" //this header allow to use set_mem_functions() to redefine original

#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

// this mock to test code if malloc returns NULL
void *mock_malloc(size_t size) {
  return NULL; // Simulate memory allocation failure
}
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
  int key = hash_time33(str, str_len);
  hashtable_add(ht, &entry->node, key);
}

static int delete_string_from_hashtable(hashtable_t *ht, const char *str) {
  // uint32_t key = hash_time33(str, strlen(str));
  int key = hash_time33(str, strlen(str));
  int bkt = calc_bkt(key, 1 << ht->bits);
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
  int key = hash_time33(str, strlen(str));
  int bkt = calc_bkt(key, 1 << ht->bits);
  string_entry_t *entry;
  hlist_for_each_entry(entry, &ht->table[bkt], node) {
    if (strcmp(entry->str, str) == 0) {
      return &entry->str;
    }
  }
  return NULL; // not found
}

void test_create_hashtable_failed(void) {
  int bits = 12;

  // mock mem functions
  set_memory_functions(mock_malloc, calloc, realloc, free);
  // null expected
  TEST_ASSERT_NULL(ht_create(bits));
  set_memory_functions(malloc, calloc, realloc, free);

  //try to create very big hashtable  
  TEST_ASSERT_NULL(ht_create(UINT32_MAX));
}

void test_create_hashtable(void) {
  int bits = 12;

  hashtable_t *ht = ht_create(bits);
  // check creation
  TEST_ASSERT_NOT_NULL(ht);

  if (ht != NULL) {
    TEST_ASSERT_EQUAL_UINT32(bits, ht->bits);
    TEST_ASSERT_NOT_NULL(ht->table);
    // free ht
    HT_FREE(ht, string_entry_t, node, free_entry);
  }
}

#define TEST_STRING "just_test_string"

void test_add_to_ht(void) {
  hashtable_t *ht = ht_create(10);
  TEST_ASSERT_NOT_NULL(ht);

  string_entry_t *entry = malloc(sizeof(string_entry_t));
  entry->str = malloc(sizeof(TEST_STRING) + 1);
  memcpy(entry->str, TEST_STRING, sizeof(TEST_STRING));
  int key = hash_time33(entry->str, sizeof(TEST_STRING));
  hashtable_add(ht, &entry->node, key);

  char **found_str = find_string_in_hashtable(ht, TEST_STRING);
  TEST_ASSERT_EQUAL_PTR(entry->str, *found_str);

  string_entry_t *found_entry = k_container_of(found_str, string_entry_t, str);
  TEST_ASSERT_EQUAL_PTR(entry, found_entry);

  // this should free everything in the hashtable
  HT_FREE(ht, string_entry_t, node, free_entry);
}

void test_delete_string_from_hashtable(void) {
  char **found_str;

  hashtable_t *ht = ht_create(10);
  TEST_ASSERT_NOT_NULL(ht);

  string_entry_t *entry = malloc(sizeof(string_entry_t));
  entry->str = malloc(sizeof(TEST_STRING) + 1);

  strcpy(entry->str, TEST_STRING);
  int key = hash_time33(entry->str, strlen(entry->str));
  hashtable_add(ht, &entry->node, key);

  found_str = find_string_in_hashtable(ht, TEST_STRING);
  TEST_ASSERT_EQUAL_PTR(entry->str, *found_str);

  int delete_result = delete_string_from_hashtable(ht, TEST_STRING);
  TEST_ASSERT_EQUAL_INT(0, delete_result);

  found_str = find_string_in_hashtable(ht, TEST_STRING);
  TEST_ASSERT_NULL(found_str);

  HT_FREE(ht, string_entry_t, node, free_entry);
}

void test_add_and_delete_entries(void) {
  hashtable_t *ht = ht_create(10);
  TEST_ASSERT_NOT_NULL(ht);

  size_t entries_before, entries_after, entries_count;
  const int num_entries = 1000;
  char str_buffer[40];

  // add entries to ht
  for (int i = 0; i < num_entries; i++) {
    snprintf(str_buffer, sizeof(str_buffer), "string%d", i);
    add_string_to_hashtable(ht, str_buffer);
  }

  // count entries and check
  COUNT_ENTRIES_IN_HASHTABLE(ht, string_entry_t, node, entries_count);
  TEST_ASSERT_EQUAL_UINT32(num_entries, entries_count);

  // delete entries and check
  for (int i = 0; i < num_entries; i++) {
    snprintf(str_buffer, sizeof(str_buffer), "string%d", i);

    // count entries before deletion
    COUNT_ENTRIES_IN_HASHTABLE(ht, string_entry_t, node, entries_before);

    int delete_result = delete_string_from_hashtable(ht, str_buffer);
    TEST_ASSERT_EQUAL_INT(0, delete_result); // ok

    // count entries after deletion
    COUNT_ENTRIES_IN_HASHTABLE(ht, string_entry_t, node, entries_after);

    // check
    TEST_ASSERT_EQUAL_UINT32(entries_before - 1, entries_after);
  }

  // free ht entries and ht itself
  HT_FREE(ht, string_entry_t, node, free_entry);
}

void test_add_delete_performance(void) {
  int ht_bits = 15;
  hashtable_t *ht = ht_create(ht_bits);
  printf("ht size: %u\n", 1 << ht->bits);
  TEST_ASSERT_NOT_NULL(ht);

  // заполняем четверть макс. емкости, чтобы минимизировать коллизии ключей
  size_t entries = (1 << ht->bits) / 4;
  printf("entries to add: %zu\n", entries);

  char test_str_prefix[] = "string";
  struct timespec start, end;
  long long time_used_for_one[500], time_used_for_all, average_time_for_all;
  size_t i, j;

  // Измеряем среднее время добавления 1 записи
  for (j = 0; j < 500; ++j) {
    clock_gettime(CLOCK_MONOTONIC, &start);
    char str[30];
    snprintf(str, sizeof(str), "%s%zu", test_str_prefix, j);
    add_string_to_hashtable(ht, str);
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_used_for_one[j] = (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
  }
  // очищаем таблицу
  HT_FREE(ht, string_entry_t, node, free_entry);

  // создаем заново
  ht = ht_create(ht_bits);

  // Вычисляем среднее время для одной записи и сравниваем
  long long total_time_for_one = 0;
  for (i = 0; i < 500; ++i) {
    // printf("%lld\n", time_used_for_one[i]);
    total_time_for_one += time_used_for_one[i];
  }
  long long avg_time_for_one = total_time_for_one / 500;

  // Наполняем таблицу и измеряем общее время добавления
  clock_gettime(CLOCK_MONOTONIC, &start);
  for (i = 0; i < entries; ++i) {
    char str[30];
    snprintf(str, sizeof(str), "%s%zu", test_str_prefix, i);
    add_string_to_hashtable(ht, str);
  }
  clock_gettime(CLOCK_MONOTONIC, &end);
  time_used_for_all = (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);

  average_time_for_all = time_used_for_all / entries;
  printf("average_time_for_one: %lldns average_time_for_all: %lldns\n", avg_time_for_one, average_time_for_all);

  if (average_time_for_all > avg_time_for_one * 1.5) {
    TEST_FAIL_MESSAGE("Performance degradation: adding entries took longer than expected (> 50%).");
  }

  // Очищаем хэш-таблицу
  for (i = 0; i < entries; ++i) {
    char str[30];
    snprintf(str, sizeof(str), "string%zu", i);
    delete_string_from_hashtable(ht, str);
  }

  size_t entries_count = 0;
  COUNT_ENTRIES_IN_HASHTABLE(ht, string_entry_t, node, entries_count);
  TEST_ASSERT_EQUAL_UINT64(0, entries_count);

  HT_FREE(ht, string_entry_t, node, free_entry);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_create_hashtable_failed);
  RUN_TEST(test_create_hashtable);
  RUN_TEST(test_add_to_ht);
  RUN_TEST(test_delete_string_from_hashtable);
  RUN_TEST(test_add_and_delete_entries);
  RUN_TEST(test_add_delete_performance);

  return UNITY_END();
}
