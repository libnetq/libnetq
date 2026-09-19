/*
 * MIT License
 *
 * Copyright (c) 2020-2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/sync/RWLock.h"

#ifdef NQ_OS_UNIX

int NQRWLock_init(NQRWLock* rwlock)
{
  return pthread_rwlock_init(rwlock, NULL);
}

int NQRWLock_destroy(NQRWLock* rwlock)
{
  return pthread_rwlock_destroy(rwlock);
}

int NQRWLock_rdlock(NQRWLock* rwlock)
{
  return pthread_rwlock_rdlock(rwlock);
}

int NQRWLock_tryrdlock(NQRWLock* rwlock)
{
  return pthread_rwlock_tryrdlock(rwlock);
}

int NQRWLock_wrlock(NQRWLock* rwlock)
{
  return pthread_rwlock_wrlock(rwlock);
}

int NQRWLock_trywrlock(NQRWLock* rwlock)
{
  return pthread_rwlock_trywrlock(rwlock);
}

int NQRWLock_unlock(NQRWLock* rwlock)
{
  return pthread_rwlock_unlock(rwlock);
}

#endif /* NQ_OS_UNIX */
