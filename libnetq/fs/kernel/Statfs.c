/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/fs/kernel/Statfs.h"

#ifdef NQ_OS_KERNEL

#include <linux/fs.h>
#include <linux/namei.h>

int NQGetStatfs(const char* path, NQStatfs* st)
{
  struct path p;
  int err;

  err = kern_path(path, LOOKUP_FOLLOW, &p);
  if (err)
    return err;

  err = vfs_statfs(&p, st);

  path_put(&p);
  return err;
}

#endif /* NQ_OS_KERNEL */
