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
typedef struct deq_head {
	struct ht_list_head list;
    uint32_t size;
} deq_head_s;

typedef struct deq {
    struct ht_list_head list;
    h_node_s *node;
} deq_s;

#define DEFINE_DEQ(name)\
    deq_head_s name;\
    name.size = 0;\
    HT_INIT_LIST_HEAD(&name.list);\

#define DEQ_PUSH_TAIL(name, max_items, entry)\
{\
    if(max_items && name.size == max_items){\
        deq_s *item = ht_list_entry(name.list.next, deq_s, list);\
        ht_list_del(name.list.next);\
        free(item);\
    }else{\
        ++name.size;\
    }\
    deq_s *deq_item = (deq_s *)malloc(sizeof(deq_s));\
    deq_item->node = entry;\
    ht_list_add_tail(&deq_item->list, &name.list);\
}\



#define DEQ_PUSH(name, max_items, entry)\
{\
    if(max_items && name.size == max_items){\
        deq_s *item = ht_list_entry(name.list.next, deq_s, list);\
        ht_list_del(name.list.next);\
        free(item);\
    }else{\
        ++name.size;\
    }\
    deq_s *deq_item = (deq_s *)malloc(sizeof(deq_s));\
    deq_item->node = entry;\
    ht_list_add(&deq_item->list, &name.list);\
}\


#define DEQ_FOR_EACH(name, deq_tmp_name, deq_member)\
    deq_s *deq_tmp_name;\
    ht_list_for_each_entry(deq_tmp_name, &name.list, deq_member)\

#define DEQ_CLEAR(name)\
{\
    deq_s *_tmp_item, *_tmp_item_next;\
    ht_list_for_each_entry_safe(_tmp_item, _tmp_item_next, &name.list, list) {\
        free(_tmp_item);\
    }\
}\

#define DEQ_POP(name, item)\
{\
    item = ht_list_entry(name.list.next, deq_s, list);\
    ht_list_del(name.list.next);\
    --name.size;\
}\

#define DEQ_POP_TAIL(name, item)\
{\
    item = ht_list_entry(name.list.prev, deq_s, list);\
    ht_list_del(name.list.prev);\
    --name.size;\
}\

#define DEQ_ISEMPTY(name) (name.size == 0 ? 1 : 0)



void _deq_push(deq_head_s *name, bool push_tail_flag, uint32_t max_items, void *entry);
void deq_push_tail(deq_head_s *name, uint32_t max_items, void *entry);
void deq_push(deq_head_s *name, uint32_t max_items, void *entry);
bool deq_isempty(deq_head_s *name);
void deq_free(deq_head_s *name);
void deq_pop(deq_head_s *name, deq_s **item);
void deq_pop_tail(deq_head_s *name, deq_s **item);


#endif