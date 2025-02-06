/*
 * Simple deque structure implementation
 * (C) 2022  Isaev Ruslan <legale.legale@gmail.com>
 */
#include <stdlib.h>
#ifdef JEMALLOC
#include "jemalloc.h"
#endif
#include "deque.h"

deq_t *deque_create(void){
  deq_t *deq = malloc(sizeof(deq_t));
  if(!deq) return NULL;
  K_INIT_LIST_HEAD(&deq->list);
  return deq;
}


void _deq_push(deq_t *name, bool push_tail_flag, void *data) {
	++name->size;
  deq_entry_t *deq_item = (deq_entry_t *)malloc(sizeof(deq_entry_t));
  deq_item->data = data;

  if (push_tail_flag) {
    k_list_add_tail(&deq_item->list, &name->list);
  } else {
    k_list_add(&deq_item->list, &name->list);
  }
}

void deq_push_tail(deq_t *name, void *data) {
  return _deq_push(name, true, data);
}

void deq_push_head(deq_t *name, void *data) {
  return _deq_push(name, false, data);
}

bool deq_isempty(deq_t *name) {
  return name->size == 0 ? 1 : 0;
}

void deq_free(deq_t *name) {
  deq_entry_t *item, *tmp;
  k_list_for_each_entry_safe(item, tmp, &name->list, list) {
    free(item);
  }
	free(name);
}

deq_entry_t *_deq_pop(deq_t *deq, bool pop_from_tail) {
    if (deq_isempty(deq)) { 
        return NULL; // if deque is empty
    }
		deq_entry_t *item;
		if(!pop_from_tail){
    	item = k_list_entry(deq->list.next, deq_entry_t, list); // get first item
    } else {
	    item = k_list_entry(deq->list.prev, deq_entry_t, list); // get first item
		}

		k_list_del(&item->list); // delete item from list
    --deq->size; // decrease deque size

    return item; // return item
}

deq_entry_t *deq_pop_tail(deq_t *deq) {
	return _deq_pop(deq, true);
}

deq_entry_t *deq_pop_head(deq_t *deq) {
	return _deq_pop(deq, false);
}


deq_entry_t *_deq_get(deq_t *deq, bool peek_from_tail) {
    if (deq_isempty(deq)) { 
        return NULL; 
    }
    deq_entry_t *item;
    if (!peek_from_tail) {
        item = k_list_entry(deq->list.next, deq_entry_t, list); 
    } else {
        item = k_list_entry(deq->list.prev, deq_entry_t, list); 
    }
    
    return item;
}

deq_entry_t *deq_get_head(deq_t *deq) {
    return _deq_get(deq, false); 
}

deq_entry_t *deq_get_tail(deq_t *deq) {
    return _deq_get(deq, true); 
}
