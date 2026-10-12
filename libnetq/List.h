/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_LIST_H
#define _LIBNETQ_LIST_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQListHead NQListHead;
struct NQListHead {
  NQListHead* next;
  NQListHead* prev;
};

#define NQ_LISTHEAD_INIT(thiz) { &(thiz), &(thiz) }
#define NQ_LISTHEAD_DEFINE(name) NQListHead name = NQ_LISTHEAD_INIT(name)

static inline void NQListHead_init(NQListHead* thiz)
{
  thiz->next = thiz;
  thiz->prev = thiz;
}

#define NQListHead_next(thiz) (thiz)->next
#define NQListHead_prev(thiz) (thiz)->prev

static inline bool NQListHead_isFirst(const NQListHead* thiz, const NQListHead* list)
{
  return thiz == list->prev;
}

static inline bool NQListHead_isLast(const NQListHead* thiz, const NQListHead* list)
{
  return thiz == list->next;
}

static inline bool NQListHead_isEmpty(const NQListHead* thiz)
{
  return thiz == thiz->next;
}

static inline void NQListHead_addFront(NQListHead* thiz, NQListHead* list)
{
  list->prev = thiz;
  list->next = thiz->next;
  list->next->prev = list;
  thiz->next = list;
}

static inline void NQListHead_addBack(NQListHead* thiz, NQListHead* list)
{
  list->next = thiz;
  list->prev = thiz->prev;
  list->prev->next = list;
  thiz->prev = list;
}

static inline void NQListHead_remove(NQListHead* thiz)
{
  NQListHead* next = thiz->next;
  NQListHead* prev = thiz->prev;

  prev->next = next;
  next->prev = prev;

  NQListHead_init(thiz);
}

static inline void NQListHead_swap(NQListHead* a, NQListHead* b)
{
  NQListHead* temp;

  if (NQListHead_isEmpty(a)) {
    if (NQListHead_isEmpty(b))
      return;
    a->next = b->next;
    a->next->prev = a;

    a->prev = b->prev;
    a->prev->next = a;

    NQListHead_init(b);
  }
  else if (NQListHead_isEmpty(b)) {
    b->next = a->next;
    b->next->prev = b;

    b->prev = a->prev;
    b->prev->next = b;

    NQListHead_init(a);
  }
  else {
    temp = a->next;
    a->next = b->next;
    a->next->prev = a;
    b->next = temp;
    b->next->prev = b;

    temp = a->prev;
    a->prev = b->prev;
    a->prev->next = a;
    b->prev = temp;
    b->prev->next = b;
  }
}

static inline void NQListHead_replace(NQListHead* oldList, NQListHead* newList)
{
  newList->next = oldList->next;
  newList->next->prev = newList;
  newList->prev = oldList->prev;
  newList->prev->next = newList;
  NQListHead_init(oldList);
}

typedef struct NQSList NQSList;
struct NQSList {
  NQSList* next;
};

static inline void NQSList_init(NQSList* thiz)
{
  thiz->next = NULL;
}

#define NQSList_next(thiz) (thiz)->next

typedef struct NQSListHead NQSListHead;
struct NQSListHead {
  NQSList* first;
  NQSList* last;
};

static inline void NQSListHead_init(NQSListHead* thiz)
{
  thiz->first = NULL;
  thiz->last = NULL;
}

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_LIST_H */
