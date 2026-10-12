/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_STUB_DIR_H
#define _LIBNETQ_FS_STUB_DIR_H

#include <libnetq/Basic.h>
#include <libnetq/ErrorCode.h>

struct NQDirImpl {
  int dummy;
};

static inline int NQDirImpl_open(struct NQDirImpl* thiz, const char* pathname)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(pathname);
  return -NQ_ENOTSUPP;
}

static inline void NQDirImpl_close(struct NQDirImpl* thiz)
{
  NQ_UNUSED_PARAM(thiz);
}

static inline bool NQDirImpl_first(struct NQDirImpl* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return false;
}

static inline bool NQDirImpl_next(struct NQDirImpl* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return false;
}

static inline const char* NQDirImpl_name(const struct NQDirImpl* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return NULL;
}

static inline bool NQDirImpl_isFile(const struct NQDirImpl* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return false;
}

static inline bool NQDirImpl_isDirectory(const struct NQDirImpl* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return false;
}

#endif /* _LIBNETQ_FS_STUB_DIR_H */
