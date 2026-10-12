/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_CLASS_NAME "NQDir"
#define NQ_LOG_TAG NQ_CLASS_NAME

#include "config.h"
#include "libnetq/fs/Dir.h"

#if defined(NQ_OS_UNIX) || defined(NQCONFIG_USE_DIRENT_H)
#include <libnetq/fs/posix/Dir.h>
#elif defined(NQ_OS_KERNEL)
#include <libnetq/fs/kernel/Dir.h>
#elif defined(NQ_OS_WINDOWS)
#include <libnetq/fs/win32/Dir.h>
#else
#include <libnetq/fs/stub/Dir.h>
#endif

#include <libnetq/Malloc.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/Assert.h>

int NQDir_open(const char* pathname, NQDir** result)
{
  struct NQDirImpl impl;

  int err = NQDirImpl_open(&impl, pathname);
  if (err) {
    *result = NULL;
    return err;
  }

  NQDir* dir = (NQDir*)NQMalloc(sizeof(*dir));
  if (dir == NULL) {
    NQDirImpl_close(&impl);
    *result = NULL;
    return -NQ_ENOMEM;
  }

  *dir = impl;
  *result = dir;
  return 0;
}

void NQDir_close(NQDir* thiz)
{
  NQDirImpl_close(thiz);
  NQFree(thiz);
}

bool NQDir_first(NQDir* thiz)
{
  return NQDirImpl_first(thiz);
}

bool NQDir_next(NQDir* thiz)
{
  return NQDirImpl_next(thiz);
}

const char* NQDir_name(NQDir* thiz)
{
  return NQDirImpl_name(thiz);
}

bool NQDir_isFile(NQDir* thiz)
{
  return NQDirImpl_isFile(thiz);
}

bool NQDir_isDirectory(NQDir* thiz)
{
  return NQDirImpl_isDirectory(thiz);
}
