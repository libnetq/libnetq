/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/sync/RWLock.h"

#ifdef NQ_OS_WINDOWS

#include <libnetq/ErrorCode.h>
#include <libnetq/Assert.h>

#define NQ_RWLOCK_MODE_NONE 0
#define NQ_RWLOCK_MODE_SHARED 1
#define NQ_RWLOCK_MODE_EXLUSIVE 2

int NQRWLock_init(NQRWLock* rwlock)
{
  InitializeSRWLock(&rwlock->native);
  rwlock->mode = NQ_RWLOCK_MODE_NONE;
  return 0;
}

int NQRWLock_destroy(NQRWLock* rwlock)
{
  NQ_UNUSED_PARAM(rwlock);
  return 0;
}

int NQRWLock_rdlock(NQRWLock* rwlock)
{
  AcquireSRWLockShared(&rwlock->native);
  rwlock->mode = NQ_RWLOCK_MODE_SHARED;
  return 0;
}

int NQRWLock_tryrdlock(NQRWLock* rwlock)
{
  if (!TryAcquireSRWLockShared(&rwlock->native))
    return -NQ_EBUSY;
  rwlock->mode = NQ_RWLOCK_MODE_SHARED;
  return 0;
}

int NQRWLock_wrlock(NQRWLock* rwlock)
{
  AcquireSRWLockExclusive(&rwlock->native);
  rwlock->mode = NQ_RWLOCK_MODE_EXLUSIVE;
  return 0;
}

int NQRWLock_trywrlock(NQRWLock* rwlock)
{
  if (!TryAcquireSRWLockExclusive(&rwlock->native))
    return -NQ_EBUSY;
  rwlock->mode = NQ_RWLOCK_MODE_EXLUSIVE;
  return 0;
}

int NQRWLock_unlock(NQRWLock* rwlock)
{
  NQ_ASSERT(rwlock->mode == NQ_RWLOCK_MODE_NONE);
  if (rwlock->mode == NQ_RWLOCK_MODE_SHARED) {
    rwlock->mode = NQ_RWLOCK_MODE_NONE;
    ReleaseSRWLockShared(&rwlock->native);
  }
  else {
    rwlock->mode = NQ_RWLOCK_MODE_NONE;
    ReleaseSRWLockExclusive(&rwlock->native);
  }
  return 0;
}

#endif /* NQ_OS_WINDOWS */
