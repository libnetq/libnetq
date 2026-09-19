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

typedef struct NQStat NQStat;
struct NQStat {
  uint32_t mode;
  uint64_t size;
  NQTimeMs accesseTimeMs;
  NQTimeMs modificationTimeMs;
  NQTimeMs creationTimeMs;
};

NQ_EXPORT int NQGetStat(const char* path, NQStat* st);

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
  return st->accesseTimeMs;
}
static inline NQTimeMs NQStat_modificationTimeMs(const NQStat* st)
{
  return st->modificationTimeMs;
}
static inline NQTimeMs NQStat_creationTimeMs(const NQStat* st)
{
  return st->creationTimeMs;
}

#ifdef __cplusplus
}
#endif

#endif
#endif /* _LIBNETQ_FS_WIN32_STAT_H */
