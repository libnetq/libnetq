/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/fs/win32/Mkdir.h"

#ifdef NQ_OS_WINDOWS

#include <libnetq/fs/Path.h>
#include <libnetq/ErrorCode.h>

#include <windows.h>

int NQMkdir(const char* path, mode_t mode)
{
  /* Windows has no POSIX permission bits; ACLs are inherited from the parent. */
  NQ_UNUSED_PARAM(mode);

  if (!path || !*path)
    return -NQ_EINVAL;

  NQWinPath winpath;
  if (!NQWinPathInit(&winpath, path))
    return -NQ_ENOMEM;

  WINBOOL success = CreateDirectoryW(winpath.characters, NULL);
  NQWinPathFinalize(&winpath);
  return success ? 0 : -NQGetLastError();
}

#endif
