/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/sync/posix/Cond.h"

#ifdef NQ_OS_UNIX

#include <libnetq/Limits.h>
#include <libnetq/Time.h>
#include <libnetq/Assert.h>

void NQCond_init(NQCond* cond)
{
  pthread_condattr_t attr;
  pthread_condattr_init(&attr);
#if HAVE_PTHREAD_CONDATTR_SETCLOCK
  pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
#endif
  pthread_cond_init(cond, &attr);
  pthread_condattr_destroy(&attr);
}

void NQCond_finalize(NQCond* cond)
{
  pthread_cond_destroy(cond);
}

void NQCond_wait(NQCond* cond, NQMutex* mutex)
{
  int result = pthread_cond_wait(cond, mutex);
  NQ_ASSERT_UNUSED(result, !result);
}

bool NQCond_waitfor(NQCond* cond, NQMutex* mutex, uint32_t ms)
{
  if (ms == NQ_UINT32_MAX) {
    NQCond_wait(cond, mutex);
    return true;
  }

  // The deadline must use the clock the condition was created with.
  NQTimeSpec ts;
#if HAVE_PTHREAD_CONDATTR_SETCLOCK
  clock_gettime(CLOCK_MONOTONIC, &ts);
#else
  clock_gettime(CLOCK_REALTIME, &ts);
#endif

  NQTimeSpec dts;
  NQTimeMsToTimeSpec(ms, &dts);
  NQTimeSpecAdd(&ts, &dts);

  return pthread_cond_timedwait(cond, mutex, &ts) == 0;
}

void NQCond_signal(NQCond* cond)
{
  int result = pthread_cond_signal(cond);
  NQ_ASSERT_UNUSED(result, !result);
}

void NQCond_broadcast(NQCond* cond)
{
  int result = pthread_cond_broadcast(cond);
  NQ_ASSERT_UNUSED(result, !result);
}

#endif /* NQ_OS_UNIX */
