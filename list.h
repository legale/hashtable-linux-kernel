
#ifndef _HT_LINUX_LIST_H
#define _HT_LINUX_LIST_H 1
/* List and hash list stuff from kernel */
#include <stddef.h>

#ifndef LIST_POISON1
#define LIST_POISON1 ((void *)0x00100100)
#endif
#ifndef LIST_POISON2
#define LIST_POISON2 ((void *)0x00200200)
#endif

struct list_head {
  struct list_head *next, *prev;
};

struct hlist_head {
  struct hlist_node *first;
};

struct hlist_node {
  struct hlist_node *next, **pprev;
};

#define ht_container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (const typeof( ((type *)0)->member ) *)(ptr);	\
	(type *)( (char *)__mptr - __builtin_offsetof(type,member) ); })

static inline void HT_INIT_LIST_HEAD(struct list_head *list) {
  list->next = list;
  list->prev = list;
}

static inline void __ht_list_add(struct list_head *new,
                                 struct list_head *prev,
                                 struct list_head *next) {
  next->prev = new;
  new->next = next;
  new->prev = prev;
  prev->next = new;
}

static inline void ht_list_add(struct list_head *new, struct list_head *head) {
  __ht_list_add(new, head, head->next);
}

static inline void ht_list_add_tail(struct list_head *new, struct list_head *head) {
  __ht_list_add(new, head->prev, head);
}

static inline void __ht_list_del(struct list_head *prev, struct list_head *next) {
  next->prev = prev;
  prev->next = next;
}

static inline void ht_list_del(struct list_head *entry) {
  __ht_list_del(entry->prev, entry->next);
}

#define ht_list_entry(ptr, type, member) \
  ht_container_of(ptr, type, member)

#define ht_list_first_entry(ptr, type, member) \
  ht_list_entry((ptr)->next, type, member)

#define ht_list_last_entry(ptr, type, member) \
  ht_list_entry((ptr)->prev, type, member)

#define ht_list_next_entry(pos, member) \
  ht_list_entry((pos)->member.next, typeof(*(pos)), member)

#define ht_list_prev_entry(pos, member) \
  ht_list_entry((pos)->member.prev, typeof(*(pos)), member)

#define ht_list_for_each_entry(pos, head, member)             \
  for (pos = ht_list_first_entry(head, typeof(*pos), member); \
       &pos->member != (head);                                \
       pos = ht_list_next_entry(pos, member))

#define ht_list_for_each_entry_safe(pos, n, head, member)     \
  for (pos = ht_list_first_entry(head, typeof(*pos), member), \
      n = ht_list_next_entry(pos, member);                    \
       &pos->member != (head);                                \
       pos = n, n = ht_list_next_entry(n, member))

#define ht_list_for_each_entry_reverse(pos, head, member)    \
  for (pos = ht_list_last_entry(head, typeof(*pos), member); \
       &pos->member != (head);                               \
       pos = ht_list_prev_entry(pos, member))

/*
 * Double linked lists with a single pointer list head.
 * Mostly useful for hash tables where the two pointer list head is
 * too wasteful.
 * You lose the ability to access the tail in O(1).
 */

#define HT_HLIST_HEAD_INIT \
  { .first = NULL }
#define HT_HLIST_HEAD(name) struct hlist_head name = {.first = NULL}
#define HT_INIT_HLIST_HEAD(ptr) ((ptr)->first = NULL)
static inline void HT_INIT_HLIST_NODE(struct hlist_node *h) {
  h->next = NULL;
  h->pprev = NULL;
}

static inline int ht_hlist_unhashed(const struct hlist_node *h) {
  return !h->pprev;
}

static inline int ht_hlist_empty(const struct hlist_head *h) {
  return !h->first;
}

static inline void __ht_hlist_del(struct hlist_node *n) {
  struct hlist_node *next = n->next;
  struct hlist_node **pprev = n->pprev;
  *pprev = next;
  if (next) {
    next->pprev = pprev;
  }
}

static inline void ht_hlist_del(struct hlist_node *n) {
  __ht_hlist_del(n);
  n->next = LIST_POISON1;
  n->pprev = LIST_POISON2;
}

static inline void ht_hlist_del_init(struct hlist_node *n) {
  if (!ht_hlist_unhashed(n)) {
    __ht_hlist_del(n);
    HT_INIT_HLIST_NODE(n);
  }
}

static inline void ht_hlist_add_head(struct hlist_node *n, struct hlist_head *h) {
  struct hlist_node *first = h->first;
  n->next = first;
  if (first)
    first->pprev = &n->next;
  h->first = n;
  n->pprev = &h->first;
}

/* next must be != NULL */
static inline void ht_hlist_add_before(struct hlist_node *n,
                                       struct hlist_node *next) {
  n->pprev = next->pprev;
  n->next = next;
  next->pprev = &n->next;
  *(n->pprev) = n;
}

static inline void ht_hlist_add_after(struct hlist_node *n,
                                      struct hlist_node *next) {
  next->next = n->next;
  n->next = next;
  next->pprev = &n->next;

  if (next->next)
    next->next->pprev = &next->next;
}

/* after that we'll appear to be on some hlist and hlist_del will work */
static inline void ht_hlist_add_fake(struct hlist_node *n) {
  n->pprev = &n->next;
}

/*
 * Move a list from one list head to another. Fixup the pprev
 * reference of the first entry if it exists.
 */
static inline void ht_hlist_move_list(struct hlist_head *old,
                                      struct hlist_head *new) {
  new->first = old->first;
  if (new->first)
    new->first->pprev = &new->first;
  old->first = NULL;
}

#define ht_hlist_entry(ptr, type, member) ht_container_of(ptr, type, member)

#define ht_hlist_for_each(pos, head) \
  for (pos = (head)->first; pos; pos = pos->next)

#define ht_hlist_for_each_safe(pos, n, head) \
  for (pos = (head)->first; pos && ({ n = pos->next; 1; });    \
       pos = n)

#define ht_hlist_entry_safe(ptr, type, member)              \
  ({                                                        \
    typeof(ptr) ____ptr = (ptr);                            \
    ____ptr ? ht_hlist_entry(____ptr, type, member) : NULL; \
  })

/**
 * hlist_for_each_entry	- iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the hlist_node within the struct.
 */
#define ht_hlist_for_each_entry(pos, head, member)                       \
  for (pos = ht_hlist_entry_safe((head)->first, typeof(*(pos)), member); \
       pos;                                                              \
       pos = ht_hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * hlist_for_each_entry_continue - iterate over a hlist continuing after current point
 * @pos:	the type * to use as a loop cursor.
 * @member:	the name of the hlist_node within the struct.
 */
#define ht_hlist_for_each_entry_continue(pos, member)                         \
  for (pos = ht_hlist_entry_safe((pos)->member.next, typeof(*(pos)), member); \
       pos;                                                                   \
       pos = ht_hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * hlist_for_each_entry_from - iterate over a hlist continuing from current point
 * @pos:	the type * to use as a loop cursor.
 * @member:	the name of the hlist_node within the struct.
 */
#define ht_hlist_for_each_entry_from(pos, member) \
  for (; pos;                                     \
       pos = ht_hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * hlist_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another &struct hlist_node to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the hlist_node within the struct.
 */
#define ht_hlist_for_each_entry_safe(pos, n, head, member)             \
  for (pos = ht_hlist_entry_safe((head)->first, typeof(*pos), member); \
       pos && ({ n = (typeof(n))pos->member.next; 1; });                                                   \
       pos = ht_hlist_entry_safe(n, typeof(*pos), member))

#endif /* _HT_LINUX_LIST_H */
