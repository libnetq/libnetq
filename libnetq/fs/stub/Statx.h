/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_STUB_STATX_H
#define _LIBNETQ_FS_STUB_STATX_H

#include <libnetq/ErrorCode.h>
#include <libnetq/Time.h>

#if !defined(NQ_OS_WINDOWS) && !defined(NQ_OS_UNIX) && !defined(NQ_OS_KERNEL)

typedef struct NQStatx NQStatx;
struct NQStatx {
  int dummy;
};

static inline NQTimeMs NQStatx_btimeMs(const NQStatx* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return 0;
}

static inline int NQGetStatx(const char* path, unsigned flags, NQStatx* st)
{
  NQ_UNUSED_PARAM(path);
  NQ_UNUSED_PARAM(flags);
  NQ_UNUSED_PARAM(st);
  return -NQ_ENOTSUPP;
}

#endif

#endif /* _LIBNETQ_FS_STUB_STATX_H */
