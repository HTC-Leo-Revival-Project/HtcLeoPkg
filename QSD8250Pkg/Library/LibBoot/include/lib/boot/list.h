/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef LIB_BOOT_INTERNAL_LIST_H
#define LIB_BOOT_INTERNAL_LIST_H

struct libboot_list_node {
    struct libboot_list_node *prev;
    struct libboot_list_node *next;
};
typedef struct libboot_list_node libboot_list_node_t;

#define libboot_containerof(ptr, type, member) \
    ((type *)((boot_uintn_t)(ptr) - LIBBOOT_OFFSETOF(type, member)))
#define LIBBOOT_LIST_INITIAL_VALUE(list) { &(list), &(list) }
#define LIBBOOT_LIST_INITIAL_CLEARED_VALUE { NULL, NULL }

static inline void libboot_list_initialize(struct libboot_list_node *list)
{
    list->prev = list->next = list;
}

static inline void libboot_list_clear_node(struct libboot_list_node *item)
{
    item->prev = item->next = 0;
}

static inline int libboot_list_in_list(struct libboot_list_node *item)
{
    if (item->prev == 0 && item->next == 0)
        return 0;
    else
        return 1;
}

static inline void libboot_list_add_head(struct libboot_list_node *list, struct libboot_list_node *item)
{
    item->next = list->next;
    item->prev = list;
    list->next->prev = item;
    list->next = item;
}

#define libboot_list_add_after(entry, new_entry) libboot_list_add_head(entry, new_entry)

static inline void libboot_list_add_tail(struct libboot_list_node *list, struct libboot_list_node *item)
{
    item->prev = list->prev;
    item->next = list;
    list->prev->next = item;
    list->prev = item;
}

#define libboot_list_add_before(entry, new_entry) libboot_list_add_tail(entry, new_entry)

static inline void libboot_list_delete(struct libboot_list_node *item)
{
    item->next->prev = item->prev;
    item->prev->next = item->next;
    item->prev = item->next = 0;
}

static inline struct libboot_list_node *libboot_list_remove_head(struct libboot_list_node *list)
{
    if (list->next != list) {
        struct libboot_list_node *item = list->next;
        libboot_list_delete(item);
        return item;
    } else {
        return NULL;
    }
}

#define libboot_list_remove_head_type(list, type, element) ({\
    struct libboot_list_node *__nod = libboot_list_remove_head(list);\
    type *__t;\
    if(__nod)\
        __t = libboot_containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct libboot_list_node *libboot_list_remove_tail(struct libboot_list_node *list)
{
    if (list->prev != list) {
        struct libboot_list_node *item = list->prev;
        libboot_list_delete(item);
        return item;
    } else {
        return NULL;
    }
}

#define libboot_list_remove_tail_type(list, type, element) ({\
    struct libboot_list_node *__nod = libboot_list_remove_tail(list);\
    type *__t;\
    if(__nod)\
        __t = libboot_containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct libboot_list_node *libboot_list_peek_head(struct libboot_list_node *list)
{
    if (list->next != list) {
        return list->next;
    } else {
        return NULL;
    }
}

#define libboot_list_peek_head_type(list, type, element) ({\
    struct libboot_list_node *__nod = libboot_list_peek_head(list);\
    type *__t;\
    if(__nod)\
        __t = libboot_containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct libboot_list_node *libboot_list_peek_tail(struct libboot_list_node *list)
{
    if (list->prev != list) {
        return list->prev;
    } else {
        return NULL;
    }
}

#define libboot_list_peek_tail_type(list, type, element) ({\
    struct libboot_list_node *__nod = libboot_list_peek_tail(list);\
    type *__t;\
    if(__nod)\
        __t = libboot_containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct libboot_list_node *libboot_list_prev(struct libboot_list_node *list, struct libboot_list_node *item)
{
    if (item->prev != list)
        return item->prev;
    else
        return NULL;
}

#define libboot_list_prev_type(list, item, type, element) ({\
    struct libboot_list_node *__nod = libboot_list_prev(list, item);\
    type *__t;\
    if(__nod)\
        __t = libboot_containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct libboot_list_node *libboot_list_prev_wrap(struct libboot_list_node *list, struct libboot_list_node *item)
{
    if (item->prev != list)
        return item->prev;
    else if (item->prev->prev != list)
        return item->prev->prev;
    else
        return NULL;
}

#define libboot_list_prev_wrap_type(list, item, type, element) ({\
    struct libboot_list_node *__nod = libboot_list_prev_wrap(list, item);\
    type *__t;\
    if(__nod)\
        __t = libboot_containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct libboot_list_node *libboot_list_next(struct libboot_list_node *list, struct libboot_list_node *item)
{
    if (item->next != list)
        return item->next;
    else
        return NULL;
}

#define libboot_list_next_type(list, item, type, element) ({\
    struct libboot_list_node *__nod = libboot_list_next(list, item);\
    type *__t;\
    if(__nod)\
        __t = libboot_containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct libboot_list_node *libboot_list_next_wrap(struct libboot_list_node *list, struct libboot_list_node *item)
{
    if (item->next != list)
        return item->next;
    else if (item->next->next != list)
        return item->next->next;
    else
        return NULL;
}

#define libboot_list_next_wrap_type(list, item, type, element) ({\
    struct libboot_list_node *__nod = libboot_list_next_wrap(list, item);\
    type *__t;\
    if(__nod)\
        __t = libboot_containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

// iterates over the list, node should be struct libboot_list_node*
#define libboot_list_for_every(list, node) \
    for(node = (list)->next; node != (list); node = node->next)

// iterates over the list in a safe way for deletion of current node
// node and temp_node should be struct libboot_list_node*
#define libboot_list_for_every_safe(list, node, temp_node) \
    for(node = (list)->next, temp_node = (node)->next;\
    node != (list);\
    node = temp_node, temp_node = (node)->next)

// iterates over the list, entry should be the container structure type *
#define libboot_list_for_every_entry(list, entry, type, member) \
    for((entry) = libboot_containerof((list)->next, type, member);\
        &(entry)->member != (list);\
        (entry) = libboot_containerof((entry)->member.next, type, member))

// iterates over the list in a safe way for deletion of current node
// entry and temp_entry should be the container structure type *
#define libboot_list_for_every_entry_safe(list, entry, temp_entry, type, member) \
    for(entry = libboot_containerof((list)->next, type, member),\
        temp_entry = libboot_containerof((entry)->member.next, type, member);\
        &(entry)->member != (list);\
        entry = temp_entry, temp_entry = libboot_containerof((temp_entry)->member.next, type, member))

static inline int libboot_list_is_empty(struct libboot_list_node *list)
{
    return (list->next == list) ? 1 : 0;
}

static inline boot_uintn_t libboot_list_length(struct libboot_list_node *list)
{
    boot_uintn_t cnt = 0;
    struct libboot_list_node *node = list;
    libboot_list_for_every(list, node) {
        cnt++;
    }

    return cnt;
}

#endif // LIB_BOOT_INTERNAL_LIST_H
