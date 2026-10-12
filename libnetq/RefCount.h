/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_REFCOUNT_H
#define _LIBNETQ_REFCOUNT_H

#include <libnetq/Atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef NQAtomic NQRefCount;

static inline void NQRefCount_init(NQRefCount* thiz)
{
  thiz->counter = 1;
}

static inline void NQRefCount_ref(NQRefCount* thiz)
{
  thiz->counter++;
}

#define NQRefCount_unref(thiz, func, ptr) \
  do {                        \
    --(thiz)->counter;        \
    if ((thiz)->counter == 0) \
      func(ptr);              \
  } while(0)

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_REFCOUNT_H */
