/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_KERNEL_STATX_H
#define _LIBNETQ_FS_KERNEL_STATX_H

#include <libnetq/Time.h>

#ifdef NQ_OS_KERNEL

#include <libnetq/fs/Stat.h>
#include <linux/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kstat NQStatx;

#define NQ_STATX_SUPPORTED 1

static inline NQTimeMs NQStatx_btimeMs(const NQStatx* thiz)
{
  if (!(thiz->result_mask & STATX_BTIME))
    return 0;
  return NQTimeSpecToTimeMs(&thiz->btime);
}

NQ_EXPORT int NQGetStatx(const char* path, unsigned flags, NQStatx* st);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NQ_OS_KERNEL */

#endif /* _LIBNETQ_FS_KERNEL_STATX_H */
