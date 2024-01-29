/*
 * Simple deque structure implementation
 * (C) 2022  Isaev Ruslan <legale.legale@gmail.com>
 */
#include <stdlib.h>

#include "deque.h"

void _deq_push(deq_head_s *name, bool push_tail_flag, uint32_t max_items, void *entry) {
  if (max_items && name->size == max_items) {
    deq_s *item = list_entry(name->list.next, deq_s, list);
    list_del(name->list.next);
    free(item);
  } else {
    ++name->size;
  }
  deq_s *deq_item = (deq_s *)malloc(sizeof(deq_s));
  deq_item->data = entry;

  if (push_tail_flag) {
    list_add_tail(&deq_item->list, &name->list);
  } else {
    list_add(&deq_item->list, &name->list);
  }
}

void deq_push_tail(deq_head_s *name, uint32_t max_items, void *entry) {
  _deq_push(name, true, max_items, entry);
}

void deq_push(deq_head_s *name, uint32_t max_items, void *entry) {
  _deq_push(name, false, max_items, entry);
}

bool deq_isempty(deq_head_s *name) {
  return name->size == 0 ? 1 : 0;
}

void deq_free(deq_head_s *name) {
  deq_s *item, *tmp;
  list_for_each_entry_safe(item, tmp, &name->list, list) {
    free(item);
  }
}

void deq_pop(deq_head_s *name, deq_s **item) {
  *item = list_entry(name->list.next, deq_s, list);
  list_del(name->list.next);
  --name->size;
}

void deq_pop_tail(deq_head_s *name, deq_s **item) {
  *item = list_entry(name->list.prev, deq_s, list);
  list_del(name->list.prev);
  --name->size;
}
