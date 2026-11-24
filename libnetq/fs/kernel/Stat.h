/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_KERNEL_STAT_H
#define _LIBNETQ_FS_KERNEL_STAT_H

#include <libnetq/Time.h>

#ifdef NQ_OS_KERNEL

# include <linux/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_STAT_IFMT  S_IFMT
#define NQ_STAT_IFREG S_IFREG
#define NQ_STAT_IFDIR S_IFDIR
#define NQ_STAT_IFLNK S_IFLNK

/* NQGetStat flags */
#define NQ_STAT_FOLLOW 0x1 /* follow symbolic links, as stat() does; without it, as lstat() */
#define NQ_STAT_EXTRA  0x2 /* also real blocks, nlink, dev and ino: without it they
                            * may be estimates on Windows (size / 512, 1, 0, 0) */

typedef struct kstat NQStat;

NQ_EXPORT int NQGetStat(const char* path, unsigned flags, NQStat* st);

/* Space allocated to the file, in 512-byte units on every platform */
static inline uint64_t NQStat_blocks(const NQStat* thiz)
{
  return (uint64_t)thiz->blocks;
}
static inline uint64_t NQStat_nlink(const NQStat* thiz)
{
  return (uint64_t)thiz->nlink;
}
static inline uint64_t NQStat_dev(const NQStat* thiz)
{
  return (uint64_t)thiz->dev;
}
static inline uint64_t NQStat_ino(const NQStat* thiz)
{
  return (uint64_t)thiz->ino;
}

static inline bool NQStat_isFile(const NQStat* st)
{
  return S_ISREG(st->mode);
}
static inline bool NQStat_isDirectory(const NQStat* st)
{
  return S_ISDIR(st->mode);
}
static inline bool NQStat_isSymbolicLink(const NQStat* st)
{
  return S_ISLNK(st->mode);
}
static inline uint64_t NQStat_size(const NQStat* st)
{
  return (uint64_t)st->size;
}
static inline NQTimeMs NQStat_accesseTimeMs(const NQStat* st)
{
  return NQTimeSpecToTimeMs(&st->atime);
}
static inline NQTimeMs NQStat_modificationTimeMs(const NQStat* st)
{
  return NQTimeSpecToTimeMs(&st->mtime);
}
static inline NQTimeMs NQStat_creationTimeMs(const NQStat* st)
{
  return NQTimeSpecToTimeMs(&st->ctime);
}

#ifdef __cplusplus
}
#endif
#endif

#endif /* _LIBNETQ_FS_KERNEL_STAT_H */
