/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_POSIX_STATX_H
#define _LIBNETQ_FS_POSIX_STATX_H

#include <libnetq/Time.h>

#ifdef NQ_OS_UNIX

#include <libnetq/fs/Stat.h>
#include <sys/stat.h>

#ifdef NQ_OS_LINUX
# include <linux/stat.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(NQ_OS_LINUX) && defined(STATX_BTIME)

typedef struct statx NQStatx;

# define NQ_STATX_SUPPORTED 1

static inline NQTimeMs NQStatx_btimeMs(const NQStatx* thiz)
{
  if (!(thiz->stx_mask & STATX_BTIME))
    return 0;
  return (NQTimeMs)thiz->stx_btime.tv_sec * 1000 + (NQTimeMs)(thiz->stx_btime.tv_nsec / 1000000);
}

#elif defined(NQ_OS_DARWIN)

typedef struct stat NQStatx;
# define NQ_STATX_SUPPORTED 1

static inline NQTimeMs NQStatx_btimeMs(const NQStatx* thiz)
{
  if (thiz->st_birthtimespec.tv_sec <= 0 && thiz->st_birthtimespec.tv_nsec <= 0)
    return 0;
  return NQTimeSpecToTimeMs(&thiz->st_birthtimespec);
}

#else

typedef struct stat NQStatx;

static inline NQTimeMs NQStatx_btimeMs(const NQStatx* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return 0;
}

#endif

NQ_EXPORT int NQGetStatx(const char* path, unsigned flags, NQStatx* st);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NQ_OS_UNIX */

#endif /* _LIBNETQ_FS_POSIX_STATX_H */
