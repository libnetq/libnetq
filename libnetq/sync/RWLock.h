/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_SYNC_RWLOCK_H
#define _LIBNETQ_SYNC_RWLOCK_H

#include <libnetq/Basic.h>

#ifdef NQ_OS_KERNEL
#include <linux/rwsem.h>
typedef struct rw_semaphore NQRWLock;
#endif

#ifdef NQ_OS_WINDOWS
#include <windows.h>
typedef struct NQRWLock {
  SRWLOCK native;
  BYTE mode;
} NQRWLock;
#endif

#ifdef NQ_OS_UNIX
#include <pthread.h>
typedef pthread_rwlock_t NQRWLock;
#endif

#ifdef __cplusplus
extern "C" {
#endif

NQ_EXPORT int NQRWLock_init(NQRWLock* rwlock);
NQ_EXPORT int NQRWLock_destroy(NQRWLock* rwlock);
NQ_EXPORT int NQRWLock_rdlock(NQRWLock* rwlock);
NQ_EXPORT int NQRWLock_tryrdlock(NQRWLock* rwlock);
NQ_EXPORT int NQRWLock_wrlock(NQRWLock* rwlock);
NQ_EXPORT int NQRWLock_trywrlock(NQRWLock* rwlock);
NQ_EXPORT int NQRWLock_unlock(NQRWLock* rwlock);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_SYNC_RWLOCK_H */
