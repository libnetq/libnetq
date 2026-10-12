/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_WIN32_STAT_H
#define _LIBNETQ_FS_WIN32_STAT_H

#include <libnetq/Time.h>

#ifdef NQ_OS_WINDOWS

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_STAT_IFMT  0170000
#define NQ_STAT_IFREG 0100000
#define NQ_STAT_IFDIR 0040000
#define NQ_STAT_IFLNK 0120000

/* NQGetStat flags */
#define NQ_STAT_FOLLOW 0x1 /* follow symbolic links, as stat() does; without it, as lstat() */
#define NQ_STAT_EXTRA  0x2 /* also real blocks, nlink, dev and ino: without it they
                            * may be estimates on Windows (size / 512, 1, 0, 0) */

typedef struct NQStat NQStat;
struct NQStat {
  uint32_t mode;
  uint64_t size;
  NQTimeSpec atime;
  NQTimeSpec mtime;
  NQTimeSpec ctime;
  uint64_t blocks; /* 512-byte units allocated: less than the size for sparse and compressed files */
  uint64_t nlink;  /* hard links */
  uint64_t dev;    /* volume serial number */
  uint64_t ino;    /* file index on the volume */
};

NQ_EXPORT int NQGetStat(const char* path, unsigned flags, NQStat* st);

/* Space allocated to the file, in 512-byte units on every platform */
static inline uint64_t NQStat_blocks(const NQStat* thiz)
{
  return thiz->blocks;
}

static inline uint64_t NQStat_nlink(const NQStat* thiz)
{
  return thiz->nlink;
}

static inline uint64_t NQStat_dev(const NQStat* thiz)
{
  return thiz->dev;
}

static inline uint64_t NQStat_ino(const NQStat* thiz)
{
  return thiz->ino;
}

static inline bool NQStat_isFile(const NQStat* st)
{
  return (st->mode & NQ_STAT_IFMT) == NQ_STAT_IFREG;
}
static inline bool NQStat_isDirectory(const NQStat* st)
{
  return (st->mode & NQ_STAT_IFMT) == NQ_STAT_IFDIR;
}
static inline bool NQStat_isSymbolicLink(const NQStat* st)
{
  return (st->mode & NQ_STAT_IFMT) == NQ_STAT_IFLNK;
}
static inline uint64_t NQStat_size(const NQStat* st)
{
  return st->size;
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
#endif /* _LIBNETQ_FS_WIN32_STAT_H */
