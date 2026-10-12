/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_WIN32_STATFS_H
#define _LIBNETQ_FS_WIN32_STATFS_H

#include <libnetq/Basic.h>

#ifdef NQ_OS_WINDOWS

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQStatfs NQStatfs;
struct NQStatfs {
  uint64_t bsize;
  uint64_t blocks;
  uint64_t bfree;
  uint64_t bavail;
  uint64_t fsid;
  uint32_t namelen;
  char typeStr[32];
};

NQ_EXPORT int NQGetStatfs(const char* path, NQStatfs* st);

static inline uint64_t NQStatfs_frsize(const NQStatfs* thiz)
{
  return thiz->bsize;
}
static inline uint64_t NQStatfs_bsize(const NQStatfs* thiz)
{
  return thiz->bsize;
}
static inline uint64_t NQStatfs_blocks(const NQStatfs* thiz)
{
  return thiz->blocks;
}
static inline uint64_t NQStatfs_bfree(const NQStatfs* thiz)
{
  return thiz->bfree;
}
static inline uint64_t NQStatfs_bavail(const NQStatfs* thiz)
{
  return thiz->bavail;
}
static inline uint64_t NQStatfs_files(const NQStatfs* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return 0;
}
static inline uint64_t NQStatfs_ffree(const NQStatfs* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return 0;
}
static inline uint64_t NQStatfs_fsid(const NQStatfs* thiz)
{
  return thiz->fsid;
}
static inline uint64_t NQStatfs_type(const NQStatfs* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return 0;
}
static inline uint32_t NQStatfs_namelen(const NQStatfs* thiz)
{
  return thiz->namelen;
}
static inline const char* NQStatfs_typeStr(const NQStatfs* thiz)
{
  return thiz->typeStr[0] ? thiz->typeStr : NULL;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NQ_OS_WINDOWS */

#endif /* _LIBNETQ_FS_WIN32_STATFS_H */
