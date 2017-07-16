/**
 * @file
 *
 * @note borrow from Linux include/linux/list.h
 */
#ifndef _WAFD_LIST_H
#define _WAFD_LIST_H

#include <stdio.h>


#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
#define prefetch(x)  (x)
#define prefetchw(x) (x)
#else
#define prefetch(x)  __builtin_prefetch(x)
#define prefetchw(x) __builtin_prefetch(x,1)
#endif

/**
 */
struct list_head {
    struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define INIT_LIST_HEAD(ptr) do { \
    (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

/**
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct list_head * entry,
    struct list_head * prev, struct list_head * next)
{
    next->prev = entry;
    entry->next = next;
    entry->prev = prev;
    prev->next = entry;
}

/**
 * add a new entry
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 *
 * @param entry new entry to be added
 * @param head list head to add it after
 */
static inline void list_add(struct list_head *entry, struct list_head *head)
{
    __list_add(entry, head, head->next);
}

/**
 * add a new entry
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 *
 * @param entry new entry to be added
 * @param head list head to add it before
 */
static inline void list_add_tail(struct list_head *entry, struct list_head *head)
{
    __list_add(entry, head->prev, head);
}

/**
 * Delete a list entry by making the prev/next entries point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct list_head * prev, struct list_head * next)
{
    next->prev = prev;
    prev->next = next;
}

/**
 * deletes entry from list.
 *
 * @param entry the element to delete from the list.
 *
 * @note list_empty on entry does not return true after this, the entry is in an undefined state.
 */
static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
}

/**
 * tests whether a list is empty
 */
static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}

/**
 * tests whether a list has just one entry.
 */
static inline int list_is_singular(const struct list_head *head)
{
    return !list_empty(head) && (head->next == head->prev);
}

static inline void __list_cut_position(struct list_head *list, struct list_head *head, struct list_head *entry)
{
    struct list_head *new_first = entry->next;
    list->next = head->next;
    list->next->prev = list;
    list->prev = entry;
    entry->next = list;
    head->next = new_first;
    new_first->prev = head;
}

/**
 * cut a list into two

 * @param list a new list to add all removed entries
 * @param head a list with entries
 * @param entry an entry within head, could be the head itself and if so we won't cut the list
 *
 * This helper moves the initial part of head, up to and
 * including entry, from head to list. You should
 * pass on entry an element you know is on head. list
 * should be an empty list or a list you do not care about
 * losing its data.
 */
static inline void list_cut_position(struct list_head *list, struct list_head *head, struct list_head *entry)
{
    if (list_empty(head)) 
        return;
    if (list_is_singular(head) && (head->next != entry && head != entry)) 
        return;
    if (entry == head) 
        INIT_LIST_HEAD(list);
    else 
        __list_cut_position(list, head, entry);
}

/**
 * get the struct for this entry
 *
 * @param ptr    the &struct list_head pointer.
 * @param type   the type of the struct this is embedded in.
 * @param member the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) container_of(ptr, type, member)

/**
 * iterate over a list
 *
 * @param pos    the &struct list_head to use as a loop counter.
 * @param head   the head for your list.
 */
#define list_for_each(pos, head) \
    for (pos = (head)->next; prefetch(pos->next), pos != (head); pos = pos->next)

/**
 * iterate over a list safe against removal of list entry
 *
 * @param pos    the &struct list_head to use as a loop counter.
 * @param n      another &struct list_head to use as temporary storage
 * @param head   the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)

/**
 * iterate over list of given type
 *
 * @param pos    the type * to use as a loop cursor.
 * @param head   the head for your list.
 * @param member the name of the list_struct within the struct.
 */
#define list_for_each_entry(pos, head, member) \
    for (pos = list_entry((head)->next, typeof(*pos), member);    \
        prefetch(pos->member.next), &pos->member != (head);       \
        pos = list_entry(pos->member.next, typeof(*pos), member))

/**
 * iterate over list of given type safe against removal of list entry
 *
 * @param pos    the type * to use as a loop cursor.
 * @param n      another type * to use as temporary storage
 * @param head   the head for your list.
 * @param member the name of the list_struct within the struct.
 */
#define list_for_each_entry_safe(pos, n, head, member)               \
    for (pos = list_entry((head)->next, typeof(*pos), member),       \
        n = list_entry(pos->member.next, typeof(*pos), member);      \
        &pos->member != (head);                                      \
        pos = n, n = list_entry(n->member.next, typeof(*n), member))

/**
 * continue iteration over list of given type
 *
 * Continue to iterate over list of given type, continuing after
 * the current position.
 *
 * @param pos    the type * to use as a loop cursor.
 * @param head   the head for your list.
 * @param member the name of the list_struct within the struct.
 */
#define list_for_each_entry_continue(pos, head, member)            \
    for (pos = list_entry(pos->member.next, typeof(*pos), member); \
         prefetch(pos->member.next), &pos->member != (head);       \
         pos = list_entry(pos->member.next, typeof(*pos), member))

/**
 * iterate over list of given type from the current point
 *
 * Iterate over list of given type, continuing from current position.
 *
 * @param pos:   the type * to use as a loop cursor.
 * @param head:  the head for your list.
 * @param member the name of the list_struct within the struct.
 */
#define list_for_each_entry_from(pos, head, member)               \
    for (; prefetch(pos->member.next), &pos->member != (head);    \
        pos = list_entry(pos->member.next, typeof(*pos), member))

/**
 * list_for_each_entry_safe_continue
 *
 * Iterate over list of given type, continuing after current point,
 * safe against removal of list entry.
 *
 * @param pos    the type * to use as a loop cursor.
 * @param n      another type * to use as temporary storage
 * @param head   the head for your list.
 * @param member the name of the list_struct within the struct.
 */
#define list_for_each_entry_safe_continue(pos, n, head, member)      \
    for (pos = list_entry(pos->member.next, typeof(*pos), member),   \
        n = list_entry(pos->member.next, typeof(*pos), member);      \
        &pos->member != (head);                                      \
        pos = n, n = list_entry(n->member.next, typeof(*n), member))

/**
 * list_for_each_entry_safe_from
 *
 * Iterate over list of given type from current point, safe against
 * removal of list entry.
 *
 * @param pos    the type * to use as a loop cursor.
 * @param n      another type * to use as temporary storage
 * @param head   the head for your list.
 * @param member the name of the list_struct within the struct.
 */
#define list_for_each_entry_safe_from(pos, n, head, member)          \
    for (n = list_entry(pos->member.next, typeof(*pos), member);     \
        &pos->member != (head);                                      \
        pos = n, n = list_entry(n->member.next, typeof(*n), member))

/*
 * Double linked lists with a single pointer list head.
 * Mostly useful for hash tables where the two pointer list head is
 * too wasteful.
 * You lose the ability to access the tail in O(1).
 */

struct hlist_head {
    struct hlist_node *first;
};

struct hlist_node {
    struct hlist_node *next, **pprev;
};

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
    struct hlist_node *next = n->next, **pprev = n->pprev;

    *pprev = next;
    if (next) next->pprev = pprev;
}

static inline void hlist_del(struct hlist_node *n)
{
    __hlist_del(n);
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
    if (first) first->pprev = &n->next;
    h->first = n;
    n->pprev = &h->first;
}

/* next must be != NULL */
static inline void hlist_add_before(struct hlist_node *n, struct hlist_node *next)
{
    n->pprev = next->pprev;
    n->next = next;
    next->pprev = &n->next;
    *(n->pprev) = n;
}

static inline void hlist_add_after(struct hlist_node *n, struct hlist_node *next)
{
    next->next = n->next;
    n->next = next;
    next->pprev = &n->next;

    if(next->next) next->next->pprev  = &next->next;
}

#define hlist_entry(ptr, type, member) container_of(ptr, type, member)

#define hlist_for_each(pos, head) \
    for (pos = (head)->first; pos && ({ prefetch(pos->next); 1; }); pos = pos->next)

#define hlist_for_each_safe(pos, n, head) \
    for (pos = (head)->first; pos && ({ n = pos->next; 1; }); pos = n)

/**
 * iterate over list of given type
 *
 * @param tpos:  the type * to use as a loop cursor.
 * @param pos:   the &struct hlist_node to use as a loop cursor.
 * @param head:  the head for your list.
 * @param member the name of the hlist_node within the struct.
 */
#define hlist_for_each_entry(tpos, pos, head, member)                \
    for (pos = (head)->first; pos && ({ prefetch(pos->next); 1;}) && \
        ({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;});     \
        pos = pos->next)

#define hlist_for_each_entry_safe(tpos, pos, n, head, member)                \
    for (pos = (head)->first; pos && ({ n = pos->next; 1;}) && \
        ({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;});     \
        pos = n)


static inline void copy_list_and_flush(struct list_head *dst,struct list_head *src)
{
    struct list_head *next = src->next;
    struct list_head *prev= src->prev;
    
    dst->next = next;
    dst->prev = prev;
    next->prev = dst;
    prev->next = dst;
    INIT_LIST_HEAD(src);
}


#ifdef __cplusplus
}
#endif

#endif
