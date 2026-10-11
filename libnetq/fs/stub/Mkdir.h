/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_STUB_MKDIR_H
#define _LIBNETQ_FS_STUB_MKDIR_H

#include <libnetq/ErrorCode.h>

typedef unsigned short mode_t;

static inline int NQMkdir(const char* path, mode_t mode)
{
  NQ_UNUSED_PARAM(path);
  NQ_UNUSED_PARAM(mode);
  return -NQ_ENOTSUPP;
}

#endif /* _LIBNETQ_FS_STUB_MKDIR_H */
