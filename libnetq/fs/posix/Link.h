/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_POSIX_LINK_H
#define _LIBNETQ_FS_POSIX_LINK_H

#include <libnetq/Basic.h>

#ifdef NQ_OS_UNIX

#include <libnetq/ErrorCode.h>
#include <unistd.h>

static inline int NQSymlink(const char* target, const char* linkPath)
{
  if (symlink(target, linkPath) != 0)
    return -NQGetLastError();
  return 0;
}

static inline int NQLink(const char* existingPath, const char* newPath)
{
  if (link(existingPath, newPath) != 0)
    return -NQGetLastError();
  return 0;
}

#endif

#endif /* _LIBNETQ_FS_POSIX_LINK_H */
