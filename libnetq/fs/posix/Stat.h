/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_POSIX_STAT_H
#define _LIBNETQ_FS_POSIX_STAT_H

#include <libnetq/Time.h>

#ifdef NQ_OS_UNIX

#include <libnetq/ErrorCode.h>
#include <sys/stat.h>

#define NQ_STAT_IFMT  S_IFMT
#define NQ_STAT_IFREG S_IFREG
#define NQ_STAT_IFDIR S_IFDIR
#define NQ_STAT_IFLNK S_IFLNK

/* NQGetStat flags */
#define NQ_STAT_FOLLOW 0x1 /* follow symbolic links, as stat() does; without it, as lstat() */
#define NQ_STAT_EXTRA  0x2 /* also real blocks, nlink, dev and ino: without it they
                            * may be estimates on Windows (size / 512, 1, 0, 0) */

typedef struct stat NQStat;

/* Looks up `path`, following symbolic links when `flags` has
 * NQ_STAT_FOLLOW. Returns 0, or a negative error code. */
static inline int NQGetStat(const char* path, unsigned flags, NQStat* st)
{
  if (((flags & NQ_STAT_FOLLOW) ? stat(path, st) : lstat(path, st)) != 0)
    return -NQGetLastError();
  return 0;
}

/* Space allocated to the file, in 512-byte units on every platform */
static inline uint64_t NQStat_blocks(const NQStat* thiz)
{
  return (uint64_t)thiz->st_blocks;
}

static inline uint64_t NQStat_nlink(const NQStat* thiz)
{
  return (uint64_t)thiz->st_nlink;
}

static inline uint64_t NQStat_dev(const NQStat* thiz)
{
  return (uint64_t)thiz->st_dev;
}

static inline uint64_t NQStat_ino(const NQStat* thiz)
{
  return (uint64_t)thiz->st_ino;
}

static inline bool NQStat_isFile(const NQStat* st)
{
  return S_ISREG(st->st_mode);
}
static inline bool NQStat_isDirectory(const NQStat* st)
{
  return S_ISDIR(st->st_mode);
}
static inline bool NQStat_isSymbolicLink(const NQStat* st)
{
  return S_ISLNK(st->st_mode);
}
static inline uint64_t NQStat_size(const NQStat* st)
{
  return (uint64_t)st->st_size;
}
static inline NQTimeMs NQStat_accesseTimeMs(const NQStat* st)
{
#ifdef NQ_OS_DARWIN
  return NQTimeSpecToTimeMs(&st->st_atimespec);
#else
  return NQTimeSpecToTimeMs(&st->st_atim);
#endif
}
static inline NQTimeMs NQStat_modificationTimeMs(const NQStat* st)
{
#ifdef NQ_OS_DARWIN
  return NQTimeSpecToTimeMs(&st->st_mtimespec);
#else
  return NQTimeSpecToTimeMs(&st->st_mtim);
#endif
}
static inline NQTimeMs NQStat_creationTimeMs(const NQStat* st)
{
#ifdef NQ_OS_DARWIN
  return NQTimeSpecToTimeMs(&st->st_ctimespec);
#else
  return NQTimeSpecToTimeMs(&st->st_ctim);
#endif
}

#endif

#endif /* _LIBNETQ_FS_POSIX_STAT_H */
