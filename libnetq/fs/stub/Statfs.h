/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_STUB_STATFS_H
#define _LIBNETQ_FS_STUB_STATFS_H

#include <libnetq/Basic.h>
#include <libnetq/ErrorCode.h>

typedef struct NQStatfs NQStatfs;
struct NQStatfs {
  int dummy;
};

static inline int NQGetStatfs(const char* path, NQStatfs* st)
{
  NQ_UNUSED_PARAM(path);
  NQ_UNUSED_PARAM(st);
  return -NQ_ENOTSUPP;
}

static inline uint64_t NQStatfs_frsize(const NQStatfs* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return 0;
}

static inline uint64_t NQStatfs_bsize(const NQStatfs* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return 0;
}

static inline uint64_t NQStatfs_blocks(const NQStatfs* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return 0;
}

static inline uint64_t NQStatfs_bfree(const NQStatfs* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return 0;
}

static inline uint64_t NQStatfs_bavail(const NQStatfs* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return 0;
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
  NQ_UNUSED_PARAM(thiz);
  return 0;
}

static inline uint64_t NQStatfs_type(const NQStatfs* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return 0;
}

static inline uint32_t NQStatfs_namelen(const NQStatfs* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return 0;
}

static inline const char* NQStatfs_typeStr(const NQStatfs* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return NULL;
}

#endif /* _LIBNETQ_FS_STUB_STATFS_H */
