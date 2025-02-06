#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "leak_detector_c.h"

#undef malloc
#undef realloc
#undef calloc
#undef free

static MEMLEAK _head;
static MEMLEAK *head = &_head;
static char memleak_flag = 0;

static inline void __add_memleak_list(MEMLEAK *new, MEMLEAK *prev, MEMLEAK *next) {
  next->prev = new;
  new->next = next;
  new->prev = prev;
  prev->next = new;
}

static inline void memleak_add_list_tail(MEMLEAK *new, MEMLEAK *head) {
  __add_memleak_list(new, head->prev, head);
}

static inline void ht_list_add(MEMLEAK *new, MEMLEAK *head) {
  __add_memleak_list(new, head, head->next);
}

static inline void __del_memleak_list(MEMLEAK *prev, MEMLEAK *next) {
  next->prev = prev;
  prev->next = next;
}

static inline void del_memleak_list(MEMLEAK *entry) {
  __del_memleak_list(entry->prev, entry->next);
  free(entry);
}

/*
 * adds allocated memory info. into the list
 *
 */
static void add(MEM_INFO alloc_info) {
  MEMLEAK *new = (MEMLEAK *)malloc(sizeof(MEMLEAK));
  new->mem_info.address = alloc_info.address;
  new->mem_info.size = alloc_info.size;
  strcpy(new->mem_info.file_name, alloc_info.file_name);
  new->mem_info.line = alloc_info.line;

  if (!memleak_flag) {
    head->prev = head;
    head->next = head;
    memleak_flag = 1;
  }

  memleak_add_list_tail(new, head);
}

/*
 * deletes all the elements from the list
 */
void clear() {
  MEMLEAK *curr = head;
  MEMLEAK *next_node;

  while (curr->next != head) {
    next_node = curr->next;
    free(curr);
    curr = next_node;
  }
}

/*
 * replacement of malloc
 */
void *xmalloc(unsigned int size, const char *file, unsigned int line) {
  void *ptr = malloc(size);
  if (ptr != NULL) {
    add_mem_info(ptr, size, file, line);
  }
  return ptr;
}

/*
 * replacement of calloc
 */
void *xcalloc(unsigned int elements, unsigned int size, const char *file, unsigned int line) {
  unsigned total_size;
  void *ptr = calloc(elements, size);
  if (ptr != NULL) {
    total_size = elements * size;
    add_mem_info(ptr, total_size, file, line);
  }
  return ptr;
}

/*
 * replacement of realloc
 */
void *xrealloc(void *ptr, unsigned int size, const char *file, unsigned int line) {
  remove_mem_info(ptr);
  ptr = realloc(ptr, size);
  add_mem_info(ptr, size, file, line);
  return ptr;
}

/*
 * replacement of free
 */
void xfree(void *mem_ref) {
  remove_mem_info(mem_ref);
  free(mem_ref);
}

/*
 * gets the allocated memory info and adds it to a list
 *
 */
void add_mem_info(void *mem_ref, unsigned int size, const char *file, unsigned int line) {
  MEM_INFO info;

  /* fill up the structure with all info */
  info.address = mem_ref;
  info.size = size;
  strncpy(info.file_name, file, FILE_NAME_LENGTH);
  info.line = line;

  /* add the above info to a list */
  add(info);
}

/*
 * if the allocated memory info is part of the list, removes it
 *
 */
void remove_mem_info(void *mem_ref) {
  MEMLEAK *curr = head;

  /* check if allocate memory is in our list */
  while (curr->next != head) {
    curr = curr->next;
    if (curr->mem_info.address == mem_ref) {
      del_memleak_list(curr);
      return;
    }
  }

  /* if mem_ref not found */
}

/*
 * writes all info of the unallocated memory into a file
 */
void report_mem_leak(void) {
  MEMLEAK *curr = head->next;
  MEMLEAK *next;

  FILE *fp_write = fopen(OUTPUT_FILE, "wt");
  char info[1024];

  if (fp_write != NULL) {
    sprintf(info, "%s\n", "Memory Leak Summary");
    fwrite(info, (strlen(info) + 1), 1, fp_write);
    sprintf(info, "%s\n", "-----------------------------------");
    fwrite(info, (strlen(info) + 1), 1, fp_write);

    while (curr->next != head) {
      printf("HIT\n");
      next = curr->next;
      sprintf(info, "address : %zu\n", (size_t)curr->mem_info.address);
      fwrite(info, (strlen(info) + 1), 1, fp_write);
      sprintf(info, "size    : %zu bytes\n", (size_t)curr->mem_info.size);
      fwrite(info, (strlen(info) + 1), 1, fp_write);
      sprintf(info, "file    : %s\n", curr->mem_info.file_name);
      fwrite(info, (strlen(info) + 1), 1, fp_write);
      sprintf(info, "line    : %d\n", curr->mem_info.line);
      fwrite(info, (strlen(info) + 1), 1, fp_write);
      sprintf(info, "%s\n", "-----------------------------------");
      fwrite(info, (strlen(info) + 1), 1, fp_write);
      curr = next;
    }
    fclose(fp_write);

  } else {
    perror("Open leak_detector_c infofile error");
  }
  clear();
}