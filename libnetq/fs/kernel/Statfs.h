/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_KERNEL_STATFS_H
#define _LIBNETQ_FS_KERNEL_STATFS_H

#include <libnetq/Basic.h>

#ifdef NQ_OS_KERNEL

#include <linux/statfs.h>

#ifdef __cplusplus
extern "C" {
#endif

NQ_EXPORT const char* NQStatfsTypeStr(uint64_t type);

typedef struct kstatfs NQStatfs;

NQ_EXPORT int NQGetStatfs(const char* path, NQStatfs* st);

static inline uint64_t NQStatfs_frsize(const NQStatfs* thiz)
{
  return thiz->f_frsize > 0 ? (uint64_t)thiz->f_frsize : (uint64_t)thiz->f_bsize;
}
static inline uint64_t NQStatfs_bsize(const NQStatfs* thiz)
{
  return (uint64_t)thiz->f_bsize;
}
static inline uint64_t NQStatfs_blocks(const NQStatfs* thiz)
{
  return (uint64_t)thiz->f_blocks;
}
static inline uint64_t NQStatfs_bfree(const NQStatfs* thiz)
{
  return (uint64_t)thiz->f_bfree;
}
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
  return ((uint64_t)(uint32_t)thiz->f_fsid.val[0] << 32) | (uint32_t)thiz->f_fsid.val[1];
}
static inline uint64_t NQStatfs_type(const NQStatfs* thiz)
{
  return (uint64_t)(uint32_t)thiz->f_type;
}
static inline uint32_t NQStatfs_namelen(const NQStatfs* thiz)
{
  return (uint32_t)thiz->f_namelen;
}
static inline const char* NQStatfs_typeStr(const NQStatfs* thiz)
{
  return NQStatfsTypeStr(NQStatfs_type(thiz));
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NQ_OS_KERNEL */

#endif /* _LIBNETQ_FS_KERNEL_STATFS_H */
