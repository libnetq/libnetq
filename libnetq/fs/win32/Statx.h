/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_WIN32_STATX_H
#define _LIBNETQ_FS_WIN32_STATX_H

#include <libnetq/Time.h>

#ifdef NQ_OS_WINDOWS

#include <libnetq/fs/Stat.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STATX_ATIME
# define STATX_ATIME 0x00000020U
#endif
#ifndef STATX_MTIME
# define STATX_MTIME 0x00000040U
#endif
#ifndef STATX_CTIME
# define STATX_CTIME 0x00000080U
#endif
#ifndef STATX_BTIME
# define STATX_BTIME 0x00000800U
#endif

typedef struct NQStatx NQStatx;
struct NQStatx {
  NQTimeSpec btime;
  NQTimeSpec atime;
  NQTimeSpec mtime;
  NQTimeSpec ctime;
  uint32_t result_mask;
};

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

#endif /* NQ_OS_WINDOWS */

#endif /* _LIBNETQ_FS_WIN32_STATX_H */
