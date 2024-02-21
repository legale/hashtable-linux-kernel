/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_K_LIST_H_
#define _LINUX_K_LIST_H_

#include "types.h"
#include "compiler.h"
#include "poison.h"
#include "container_of.h"

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

#define K_LIST_HEAD_INIT(name) { &(name), &(name) }

#define K_LIST_HEAD(name) \
	struct k_list_head name = K_LIST_HEAD_INIT(name)

static inline void K_INIT_LIST_HEAD(struct k_list_head *list)
{
	list->next = list;
	list->prev = list;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
#ifndef CONFIG_DEBUG_LIST
static inline void __list_add(struct k_list_head *new,
			      struct k_list_head *prev,
			      struct k_list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}
#else
extern void __list_add(struct k_list_head *new,
			      struct k_list_head *prev,
			      struct k_list_head *next);
#endif

/**
 * k_list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void k_list_add(struct k_list_head *new, struct k_list_head *head)
{
	__list_add(new, head, head->next);
}


/**
 * k_list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void k_list_add_tail(struct k_list_head *new, struct k_list_head *head)
{
	__list_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct k_list_head * prev, struct k_list_head * next)
{
	next->prev = prev;
	WRITE_ONCE(prev->next, next);
}

/**
 * k_list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: k_list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
#ifndef CONFIG_DEBUG_LIST
static inline void __list_del_entry(struct k_list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

static inline void k_list_del(struct k_list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}
#else
extern void __list_del_entry(struct k_list_head *entry);
extern void k_list_del(struct k_list_head *entry);
#endif

/**
 * k_list_replace - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void k_list_replace(struct k_list_head *old,
				struct k_list_head *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

static inline void k_list_replace_init(struct k_list_head *old,
					struct k_list_head *new)
{
	k_list_replace(old, new);
	K_INIT_LIST_HEAD(old);
}

/**
 * k_list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void k_list_del_init(struct k_list_head *entry)
{
	__list_del_entry(entry);
	K_INIT_LIST_HEAD(entry);
}

/**
 * k_list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void k_list_move(struct k_list_head *list, struct k_list_head *head)
{
	__list_del_entry(list);
	k_list_add(list, head);
}

/**
 * k_list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void k_list_move_tail(struct k_list_head *list,
				  struct k_list_head *head)
{
	__list_del_entry(list);
	k_list_add_tail(list, head);
}

/**
 * k_list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int k_list_is_last(const struct k_list_head *list,
				const struct k_list_head *head)
{
	return list->next == head;
}

/**
 * k_list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int k_list_empty(const struct k_list_head *head)
{
	return head->next == head;
}

/**
 * k_list_empty_careful - tests whether a list is empty and not being modified
 * @head: the list to test
 *
 * Description:
 * tests whether a list is empty _and_ checks that no other CPU might be
 * in the process of modifying either member (next or prev)
 *
 * NOTE: using k_list_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the list entry is k_list_del_init(). Eg. it cannot be used
 * if another CPU could re-k_list_add() it.
 */
static inline int k_list_empty_careful(const struct k_list_head *head)
{
	struct k_list_head *next = head->next;
	return (next == head) && (next == head->prev);
}

/**
 * k_list_rotate_left - rotate the list to the left
 * @head: the head of the list
 */
static inline void k_list_rotate_left(struct k_list_head *head)
{
	struct k_list_head *first;

	if (!k_list_empty(head)) {
		first = head->next;
		k_list_move_tail(first, head);
	}
}

/**
 * k_list_is_singular - tests whether a list has just one entry.
 * @head: the list to test.
 */
static inline int k_list_is_singular(const struct k_list_head *head)
{
	return !k_list_empty(head) && (head->next == head->prev);
}

static inline void __list_cut_position(struct k_list_head *list,
		struct k_list_head *head, struct k_list_head *entry)
{
	struct k_list_head *new_first = entry->next;
	list->next = head->next;
	list->next->prev = list;
	list->prev = entry;
	entry->next = list;
	head->next = new_first;
	new_first->prev = head;
}

/**
 * k_list_cut_position - cut a list into two
 * @list: a new list to add all removed entries
 * @head: a list with entries
 * @entry: an entry within head, could be the head itself
 *	and if so we won't cut the list
 *
 * This helper moves the initial part of @head, up to and
 * including @entry, from @head to @list. You should
 * pass on @entry an element you know is on @head. @list
 * should be an empty list or a list you do not care about
 * losing its data.
 *
 */
static inline void k_list_cut_position(struct k_list_head *list,
		struct k_list_head *head, struct k_list_head *entry)
{
	if (k_list_empty(head))
		return;
	if (k_list_is_singular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
		K_INIT_LIST_HEAD(list);
	else
		__list_cut_position(list, head, entry);
}

static inline void __list_splice(const struct k_list_head *list,
				 struct k_list_head *prev,
				 struct k_list_head *next)
{
	struct k_list_head *first = list->next;
	struct k_list_head *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 * k_list_splice - join two lists, this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void k_list_splice(const struct k_list_head *list,
				struct k_list_head *head)
{
	if (!k_list_empty(list))
		__list_splice(list, head, head->next);
}

/**
 * k_list_splice_tail - join two lists, each list being a queue
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void k_list_splice_tail(struct k_list_head *list,
				struct k_list_head *head)
{
	if (!k_list_empty(list))
		__list_splice(list, head->prev, head);
}

/**
 * k_list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static inline void k_list_splice_init(struct k_list_head *list,
				    struct k_list_head *head)
{
	if (!k_list_empty(list)) {
		__list_splice(list, head, head->next);
		K_INIT_LIST_HEAD(list);
	}
}

/**
 * k_list_splice_tail_init - join two lists and reinitialise the emptied list
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * Each of the lists is a queue.
 * The list at @list is reinitialised
 */
static inline void k_list_splice_tail_init(struct k_list_head *list,
					 struct k_list_head *head)
{
	if (!k_list_empty(list)) {
		__list_splice(list, head->prev, head);
		K_INIT_LIST_HEAD(list);
	}
}

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct k_list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the k_list_head within the struct.
 */
#define k_list_entry(ptr, type, member) \
	k_container_of(ptr, type, member)

/**
 * k_list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the k_list_head within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define k_list_first_entry(ptr, type, member) \
	k_list_entry((ptr)->next, type, member)

/**
 * k_list_last_entry - get the last element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the k_list_head within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define k_list_last_entry(ptr, type, member) \
	k_list_entry((ptr)->prev, type, member)

/**
 * k_list_first_entry_or_null - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the k_list_head within the struct.
 *
 * Note that if the list is empty, it returns NULL.
 */
#define k_list_first_entry_or_null(ptr, type, member) \
	(!k_list_empty(ptr) ? k_list_first_entry(ptr, type, member) : NULL)

/**
 * k_list_last_entry_or_null - get the last element from a list
 * @ptr:       the list head to take the element from.
 * @type:      the type of the struct this is embedded in.
 * @member:    the name of the k_list_head within the struct.
 *
 * Note that if the list is empty, it returns NULL.
 */
#define k_list_last_entry_or_null(ptr, type, member) \
	(!k_list_empty(ptr) ? k_list_last_entry(ptr, type, member) : NULL)

/**
 * k_list_next_entry - get the next element in list
 * @pos:	the type * to cursor
 * @member:	the name of the k_list_head within the struct.
 */
#define k_list_next_entry(pos, member) \
	k_list_entry((pos)->member.next, typeof(*(pos)), member)

/**
 * k_list_prev_entry - get the prev element in list
 * @pos:	the type * to cursor
 * @member:	the name of the k_list_head within the struct.
 */
#define k_list_prev_entry(pos, member) \
	k_list_entry((pos)->member.prev, typeof(*(pos)), member)

/**
 * k_list_for_each	-	iterate over a list
 * @pos:	the &struct k_list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define k_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * k_list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct k_list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define k_list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * k_list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the &struct k_list_head to use as a loop cursor.
 * @n:		another &struct k_list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define k_list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * k_list_for_each_prev_safe - iterate over a list backwards safe against removal of list entry
 * @pos:	the &struct k_list_head to use as a loop cursor.
 * @n:		another &struct k_list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define k_list_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
	     pos != (head); \
	     pos = n, n = pos->prev)

/**
 * k_list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the k_list_head within the struct.
 */
#define k_list_for_each_entry(pos, head, member)				\
	for (pos = k_list_first_entry(head, typeof(*pos), member);	\
	     &pos->member != (head);					\
	     pos = k_list_next_entry(pos, member))

/**
 * k_list_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the k_list_head within the struct.
 */
#define k_list_for_each_entry_reverse(pos, head, member)			\
	for (pos = k_list_last_entry(head, typeof(*pos), member);		\
	     &pos->member != (head); 					\
	     pos = k_list_prev_entry(pos, member))

/**
 * k_list_prepare_entry - prepare a pos entry for use in k_list_for_each_entry_continue()
 * @pos:	the type * to use as a start point
 * @head:	the head of the list
 * @member:	the name of the k_list_head within the struct.
 *
 * Prepares a pos entry for use as a start point in k_list_for_each_entry_continue().
 */
#define k_list_prepare_entry(pos, head, member) \
	((pos) ? : k_list_entry(head, typeof(*pos), member))

/**
 * k_list_for_each_entry_continue - continue iteration over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the k_list_head within the struct.
 *
 * Continue to iterate over list of given type, continuing after
 * the current position.
 */
#define k_list_for_each_entry_continue(pos, head, member) 		\
	for (pos = k_list_next_entry(pos, member);			\
	     &pos->member != (head);					\
	     pos = k_list_next_entry(pos, member))

/**
 * k_list_for_each_entry_continue_reverse - iterate backwards from the given point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the k_list_head within the struct.
 *
 * Start to iterate over list of given type backwards, continuing after
 * the current position.
 */
#define k_list_for_each_entry_continue_reverse(pos, head, member)		\
	for (pos = k_list_prev_entry(pos, member);			\
	     &pos->member != (head);					\
	     pos = k_list_prev_entry(pos, member))

/**
 * k_list_for_each_entry_from - iterate over list of given type from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the k_list_head within the struct.
 *
 * Iterate over list of given type, continuing from current position.
 */
#define k_list_for_each_entry_from(pos, head, member) 			\
	for (; &pos->member != (head);					\
	     pos = k_list_next_entry(pos, member))

/**
 * k_list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the k_list_head within the struct.
 */
#define k_list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = k_list_first_entry(head, typeof(*pos), member),	\
		n = k_list_next_entry(pos, member);			\
	     &pos->member != (head); 					\
	     pos = n, n = k_list_next_entry(n, member))

/**
 * k_list_for_each_entry_safe_continue - continue list iteration safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the k_list_head within the struct.
 *
 * Iterate over list of given type, continuing after current point,
 * safe against removal of list entry.
 */
#define k_list_for_each_entry_safe_continue(pos, n, head, member) 		\
	for (pos = k_list_next_entry(pos, member), 				\
		n = k_list_next_entry(pos, member);				\
	     &pos->member != (head);						\
	     pos = n, n = k_list_next_entry(n, member))

/**
 * k_list_for_each_entry_safe_from - iterate over list from current point safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the k_list_head within the struct.
 *
 * Iterate over list of given type from current point, safe against
 * removal of list entry.
 */
#define k_list_for_each_entry_safe_from(pos, n, head, member) 			\
	for (n = k_list_next_entry(pos, member);					\
	     &pos->member != (head);						\
	     pos = n, n = k_list_next_entry(n, member))

/**
 * k_list_for_each_entry_safe_reverse - iterate backwards over list safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the k_list_head within the struct.
 *
 * Iterate backwards over list of given type, safe against removal
 * of list entry.
 */
#define k_list_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = k_list_last_entry(head, typeof(*pos), member),		\
		n = k_list_prev_entry(pos, member);			\
	     &pos->member != (head); 					\
	     pos = n, n = k_list_prev_entry(n, member))

/**
 * k_list_safe_reset_next - reset a stale k_list_for_each_entry_safe loop
 * @pos:	the loop cursor used in the k_list_for_each_entry_safe loop
 * @n:		temporary storage used in k_list_for_each_entry_safe
 * @member:	the name of the k_list_head within the struct.
 *
 * k_list_safe_reset_next is not safe to use in general if the list may be
 * modified concurrently (eg. the lock is dropped in the loop body). An
 * exception to this is if the cursor element (pos) is pinned in the list,
 * and k_list_safe_reset_next is called after re-taking the lock and before
 * completing the current iteration of the loop body.
 */
#define k_list_safe_reset_next(pos, n, member)				\
	n = k_list_next_entry(pos, member)

/*
 * Double linked lists with a single pointer list head.
 * Mostly useful for hash tables where the two pointer list head is
 * too wasteful.
 * You lose the ability to access the tail in O(1).
 */

#define HLIST_HEAD_INIT { .first = NULL }
#define HLIST_HEAD(name) struct hlist_head name = {  .first = NULL }
#define INIT_HLIST_HEAD(ptr) ((ptr)->first = NULL)
static inline void INIT_HLIST_NODE(struct hlist_node *h)
{
	h->next = NULL;
	h->pprev = NULL;
}

static inline int hlist_unhashed(const struct hlist_node *h)
{
	return !h->pprev;
}

static inline int hlist_empty(const struct hlist_head *h)
{
	return !h->first;
}

static inline void __hlist_del(struct hlist_node *n)
{
	struct hlist_node *next = n->next;
	struct hlist_node **pprev = n->pprev;

	WRITE_ONCE(*pprev, next);
	if (next)
		next->pprev = pprev;
}

static inline void hlist_del(struct hlist_node *n)
{
	__hlist_del(n);
	n->next = LIST_POISON1;
	n->pprev = LIST_POISON2;
}

static inline void hlist_del_init(struct hlist_node *n)
{
	if (!hlist_unhashed(n)) {
		__hlist_del(n);
		INIT_HLIST_NODE(n);
	}
}

static inline void hlist_add_head(struct hlist_node *n, struct hlist_head *h)
{
	struct hlist_node *first = h->first;
	n->next = first;
	if (first)
		first->pprev = &n->next;
	h->first = n;
	n->pprev = &h->first;
}

/* next must be != NULL */
static inline void hlist_add_before(struct hlist_node *n,
					struct hlist_node *next)
{
	n->pprev = next->pprev;
	n->next = next;
	next->pprev = &n->next;
	*(n->pprev) = n;
}

static inline void hlist_add_behind(struct hlist_node *n,
				    struct hlist_node *prev)
{
	n->next = prev->next;
	prev->next = n;
	n->pprev = &prev->next;

	if (n->next)
		n->next->pprev  = &n->next;
}

/* after that we'll appear to be on some hlist and hlist_del will work */
static inline void hlist_add_fake(struct hlist_node *n)
{
	n->pprev = &n->next;
}

static inline bool hlist_fake(struct hlist_node *h)
{
	return h->pprev == &h->next;
}

/*
 * Move a list from one list head to another. Fixup the pprev
 * reference of the first entry if it exists.
 */
static inline void hlist_move_list(struct hlist_head *old,
				   struct hlist_head *new)
{
	new->first = old->first;
	if (new->first)
		new->first->pprev = &new->first;
	old->first = NULL;
}

#define hlist_entry(ptr, type, member) k_container_of(ptr,type,member)

#define hlist_for_each(pos, head) \
	for (pos = (head)->first; pos ; pos = pos->next)

#define hlist_for_each_safe(pos, n, head) \
	for (pos = (head)->first; pos && ({ n = pos->next; 1; }); \
	     pos = n)

#define hlist_entry_safe(ptr, type, member) \
	({ typeof(ptr) ____ptr = (ptr); \
	   ____ptr ? hlist_entry(____ptr, type, member) : NULL; \
	})

/**
 * hlist_for_each_entry	- iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the hlist_node within the struct.
 */
#define hlist_for_each_entry(pos, head, member)				\
	for (pos = hlist_entry_safe((head)->first, typeof(*(pos)), member);\
	     pos;							\
	     pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * hlist_for_each_entry_continue - iterate over a hlist continuing after current point
 * @pos:	the type * to use as a loop cursor.
 * @member:	the name of the hlist_node within the struct.
 */
#define hlist_for_each_entry_continue(pos, member)			\
	for (pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member);\
	     pos;							\
	     pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * hlist_for_each_entry_from - iterate over a hlist continuing from current point
 * @pos:	the type * to use as a loop cursor.
 * @member:	the name of the hlist_node within the struct.
 */
#define hlist_for_each_entry_from(pos, member)				\
	for (; pos;							\
	     pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * hlist_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another &struct hlist_node to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the hlist_node within the struct.
 */
#define hlist_for_each_entry_safe(pos, n, head, member) 		\
	for (pos = hlist_entry_safe((head)->first, typeof(*pos), member);\
	     pos && ({ n = pos->member.next; 1; });			\
	     pos = hlist_entry_safe(n, typeof(*pos), member))

/**
 * list_del_range - deletes range of entries from list.
 * @begin: first element in the range to delete from the list.
 * @end: last element in the range to delete from the list.
 * Note: k_list_empty on the range of entries does not return true after this,
 * the entries is in an undefined state.
 */
static inline void list_del_range(struct k_list_head *begin,
				  struct k_list_head *end)
{
	begin->prev->next = end->next;
	end->next->prev = begin->prev;
}

/**
 * list_for_each_from	-	iterate over a list from one of its nodes
 * @pos:  the &struct k_list_head to use as a loop cursor, from where to start
 * @head: the head for your list.
 */
#define list_for_each_from(pos, head) \
	for (; pos != (head); pos = pos->next)

#endif /* _LINUX_K_LIST_H_ */