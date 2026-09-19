/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/sync/RWLock.h"

#ifdef NQ_OS_KERNEL

#include <libnetq/ErrorCode.h>

int NQRWLock_init(NQRWLock* rwlock)
{
  init_rwsem(rwlock);
  return 0;
}

int NQRWLock_destroy(NQRWLock* rwlock)
{
  return 0;
}

int NQRWLock_rdlock(NQRWLock* rwlock)
{
  down_read(rwlock);
  return 0;
}

int NQRWLock_tryrdlock(NQRWLock* rwlock)
{
  return down_read_trylock(rwlock) ? 0 : -NQ_EBUSY;
}

int NQRWLock_wrlock(NQRWLock* rwlock)
{
  down_write(rwlock);
  return 0;
}

int NQRWLock_trywrlock(NQRWLock* rwlock)
{
  return down_write_trylock(rwlock) ? 0 : -NQ_EBUSY;
}

int NQRWLock_unlock(NQRWLock* rwlock)
{
  if (rwsem_is_locked(rwlock)) {
    if (atomic_long_read(&rwlock->owner))
      up_write(rwlock);
    else
      up_read(rwlock);
  }
  return 0;
}

#endif /* NQ_OS_KERNEL */
