/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_STUB_LINK_H
#define _LIBNETQ_FS_STUB_LINK_H

#include <libnetq/ErrorCode.h>

static inline int NQSymlink(const char* target, const char* linkPath)
{
  NQ_UNUSED_PARAM(target);
  NQ_UNUSED_PARAM(linkPath);
  return -NQ_ENOTSUPP;
}

static inline int NQLink(const char* existingPath, const char* newPath)
{
  NQ_UNUSED_PARAM(existingPath);
  NQ_UNUSED_PARAM(newPath);
  return -NQ_ENOTSUPP;
}

#endif /* _LIBNETQ_FS_STUB_LINK_H */
