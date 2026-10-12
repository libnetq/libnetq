/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_SYNC_STUB_COND_H
#define _LIBNETQ_SYNC_STUB_COND_H

#include <libnetq/Mutex.h>

typedef struct NQCond NQCond;
struct NQCond {
  int dummy;
};

static inline void NQCond_init(NQCond* thiz)
{
  NQ_UNUSED_PARAM(thiz);
}

void NQCond_finalize(NQCond* thiz)
{
  NQ_UNUSED_PARAM(thiz);
}

void NQCond_wait(NQCond* thiz, NQMutex* mutex)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(mutex);
}

bool NQCond_waitfor(NQCond* thiz, NQMutex* mutex, uint32_t ms)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(mutex);
  NQ_UNUSED_PARAM(ms);
  return false;
}

void NQCond_signal(NQCond* thiz)
{
  NQ_UNUSED_PARAM(thiz);
}

void NQCond_broadcast(NQCond* thiz)
{
  NQ_UNUSED_PARAM(thiz);
}

#endif /* _LIBNETQ_SYNC_STUB_COND_H */
