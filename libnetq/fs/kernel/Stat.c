/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/fs/kernel/Stat.h"

#ifdef NQ_OS_KERNEL

#include <linux/namei.h>
#include <linux/fs.h>

int NQGetStat(const char* path, unsigned flags, NQStat* st)
{
  struct path p;
  int err;

  err = kern_path(path, (flags & NQ_STAT_FOLLOW) ? LOOKUP_FOLLOW : 0, &p);
  if (err)
    return err;

  err = vfs_getattr(&p, st, STATX_BASIC_STATS, AT_STATX_SYNC_AS_STAT);

  path_put(&p);
  return err;
}

#endif
