/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_POSIX_STATFS_H
#define _LIBNETQ_FS_POSIX_STATFS_H

#include <libnetq/Basic.h>

#ifdef NQ_OS_UNIX

#include <libnetq/ErrorCode.h>
#include <libnetq/string/String.h>

#if defined(NQ_OS_LINUX)
# include <sys/vfs.h>
#elif defined(NQ_OS_DARWIN)
# include <limits.h>
# include <sys/mount.h>
# include <sys/param.h>
#else
# include <sys/statvfs.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

NQ_EXPORT const char* NQStatfsTypeStr(uint64_t type);

#if defined(NQ_OS_LINUX) || defined(NQ_OS_DARWIN)

typedef struct statfs NQStatfs;

static inline int NQGetStatfs(const char* path, NQStatfs* st)
{
  if (statfs(path, st) != 0)
    return -NQGetLastError();
  return 0;
}

#else

typedef struct statvfs NQStatfs;

static inline int NQGetStatfs(const char* path, NQStatfs* st)
{
  if (statvfs(path, st) != 0)
    return -NQGetLastError();
  return 0;
}

#endif

static inline uint64_t NQStatfs_frsize(const NQStatfs* thiz)
{
#if defined(NQ_OS_DARWIN)
  return (uint64_t)thiz->f_bsize;
#else
  return thiz->f_frsize > 0 ? (uint64_t)thiz->f_frsize : (uint64_t)thiz->f_bsize;
#endif
}

static inline uint64_t NQStatfs_bsize(const NQStatfs* thiz)
{
#if defined(NQ_OS_DARWIN)
  return thiz->f_iosize > 0 ? (uint64_t)thiz->f_iosize : (uint64_t)thiz->f_bsize;
#else
  return (uint64_t)thiz->f_bsize;
#endif
}

static inline uint64_t NQStatfs_blocks(const NQStatfs* thiz)
{
  return (uint64_t)thiz->f_blocks;
}
static inline uint64_t NQStatfs_bfree(const NQStatfs* thiz)
{
  return (uint64_t)thiz->f_bfree;
}
/* free to an unprivileged user */
static inline uint64_t NQStatfs_bavail(const NQStatfs* thiz)
{
  return (uint64_t)thiz->f_bavail;
}
static inline uint64_t NQStatfs_files(const NQStatfs* thiz)
{
  return (uint64_t)thiz->f_files;
}
static inline uint64_t NQStatfs_ffree(const NQStatfs* thiz)
{
  return (uint64_t)thiz->f_ffree;
}

static inline uint64_t NQStatfs_fsid(const NQStatfs* thiz)
{
#if defined(NQ_OS_LINUX) || defined(NQ_OS_DARWIN)
  uint32_t words[2] = {0, 0};
  NQMemcpy(words, &thiz->f_fsid, sizeof(thiz->f_fsid) < sizeof(words) ? sizeof(thiz->f_fsid) : sizeof(words));
  return ((uint64_t)words[0] << 32) | words[1];
#else
  return (uint64_t)thiz->f_fsid;
#endif
}

static inline uint64_t NQStatfs_type(const NQStatfs* thiz)
{
#if defined(NQ_OS_LINUX)
  return (uint64_t)(uint32_t)thiz->f_type;
#elif defined(NQ_OS_DARWIN)
  return (uint64_t)thiz->f_type;
#else
  NQ_UNUSED_PARAM(thiz);
  return 0;
#endif
}

static inline uint32_t NQStatfs_namelen(const NQStatfs* thiz)
{
#if defined(NQ_OS_LINUX)
  return (uint32_t)thiz->f_namelen;
#elif defined(NQ_OS_DARWIN)
  NQ_UNUSED_PARAM(thiz);
  return NAME_MAX; /* statfs keeps none; APFS and HFS+ take 255 */
#else
  return (uint32_t)thiz->f_namemax;
#endif
}

static inline const char* NQStatfs_typeStr(const NQStatfs* thiz)
{
#if defined(NQ_OS_LINUX)
  return NQStatfsTypeStr(NQStatfs_type(thiz));
#elif defined(NQ_OS_DARWIN)
  return thiz->f_fstypename[0] ? thiz->f_fstypename : NULL;
#else
  NQ_UNUSED_PARAM(thiz);
  return NULL;
#endif
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NQ_OS_UNIX */

#endif /* _LIBNETQ_FS_POSIX_STATFS_H */
