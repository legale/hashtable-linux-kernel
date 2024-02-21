/*
 * Simple deque structure implementation
 * (C) 2022  Isaev Ruslan <legale.legale@gmail.com>
 */

#ifndef __DEQUE_H__
#define __DEQUE_H__
#include <stdint.h>
#include <stdbool.h>

#include "list.h"
#include "hashtable.h"


/* this structure to store entries as deque structure */
typedef struct deq {
	struct k_list_head list;
    int size;
} deq_t;

typedef struct deq_entry {
    struct k_list_head list;
    void *data;
} deq_entry_t;

#define DEFINE_DEQ(name)\
    deq_t name;\
    name.size = 0;\
    K_INIT_LIST_HEAD(&name.list);\

#define DEQ_PUSH_TAIL(name, max_items, entry)\
{\
    if(max_items && name.size == max_items){\
        deq_entry_t *item = k_list_entry(name.list.next, deq_entry_t, list);\
        k_list_del(name.list.next);\
        free(item);\
    }else{\
        ++name.size;\
    }\
    deq_entry_t *deq_item = (deq_entry_t *)malloc(sizeof(deq_entry_t));\
    deq_item->node = entry;\
    k_list_add_tail(&deq_item->list, &name.list);\
}\



#define DEQ_PUSH(name, max_items, entry)\
{\
    if(max_items && name.size == max_items){\
        deq_entry_t *item = k_list_entry(name.list.next, deq_entry_t, list);\
        k_list_del(name.list.next);\
        free(item);\
    }else{\
        ++name.size;\
    }\
    deq_entry_t *deq_item = (deq_entry_t *)malloc(sizeof(deq_entry_t));\
    deq_item->node = entry;\
    k_list_add(&deq_item->list, &name.list);\
}\


#define DEQ_FOR_EACH(deq_struct, pos, deq_member)\
    k_list_for_each_entry(pos, &deq_struct.list, deq_member)\

#define DEQ_CLEAR(name)\
{\
    deq_entry_t *_tmp_item, *_tmp_item_next;\
    k_list_for_each_entry_safe(_tmp_item, _tmp_item_next, &name.list, list) {\
        free(_tmp_item);\
    }\
}\

#define DEQ_POP(name, item)\
{\
    item = k_list_entry(name.list.next, deq_entry_t, list);\
    k_list_del(name.list.next);\
    --name.size;\
}\

#define DEQ_POP_TAIL(name, item)\
{\
    item = k_list_entry(name.list.prev, deq_entry_t, list);\
    k_list_del(name.list.prev);\
    --name.size;\
}\

#define DEQ_ISEMPTY(name) (name.size == 0 ? 1 : 0)



deq_t *deque_create(void);
bool deq_isempty(deq_t *name);
void deq_free(deq_t *name);

void _deq_push(deq_t *name, bool push_tail_flag, void *data);
void deq_push_tail(deq_t *name, void *data);
void deq_push_head(deq_t *name, void *data);

deq_entry_t *deq_pop_tail(deq_t *name);
deq_entry_t *deq_pop_head(deq_t *name);
deq_entry_t *deq_get_head(deq_t *deq);
deq_entry_t *deq_get_tail(deq_t *deq);

#endif