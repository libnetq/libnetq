/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_POSIX_MKDIR_H
#define _LIBNETQ_FS_POSIX_MKDIR_H

#include <libnetq/Basic.h>

#ifdef NQ_OS_UNIX

#include <libnetq/ErrorCode.h>
#include <sys/stat.h>

static inline int NQMkdir(const char* path, mode_t mode)
{
  if (mkdir(path, mode) != 0)
    return -NQGetLastError();
  return 0;
}

#endif

#endif /* _LIBNETQ_FS_POSIX_MKDIR_H */
