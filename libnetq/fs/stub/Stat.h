/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_STUB_STAT_H
#define _LIBNETQ_FS_STUB_STAT_H

#include <libnetq/Time.h>
#include <libnetq/ErrorCode.h>

typedef struct NQStat NQStat;
struct NQStat {
  int dummy;
};

static inline int NQGetStat(const char* path, NQStat* st)
{
  NQ_UNUSED_PARAM(path);
  NQ_UNUSED_PARAM(st);
  return -NQ_ENOTSUPP;
}

static inline bool NQStat_isFile(const NQStat* st)
{
  return false;
}
static inline bool NQStat_isDirectory(const NQStat* st)
{
  return false;
}
static inline bool NQStat_isSymbolicLink(const NQStat* st)
{
  return false;
}
static inline uint64_t NQStat_size(const NQStat* st)
{
  return 0;
}
static inline NQTimeMs NQStat_accesseTimeMs(const NQStat* st)
{
  return 0;
}
static inline NQTimeMs NQStat_modificationTimeMs(const NQStat* st)
{
  return 0;
}
static inline NQTimeMs NQStat_creationTimeMs(const NQStat* st)
{
  return 0;
}

#endif /* _LIBNETQ_FS_STUB_STAT_H */
