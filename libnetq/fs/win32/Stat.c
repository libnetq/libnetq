/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/fs/win32/Stat.h"

#ifdef NQ_OS_WINDOWS

#include <libnetq/fs/Path.h>
#include <libnetq/Time.h>
#include <libnetq/Assert.h>

#include <windows.h>

int NQGetStat(const char* path, NQStat* st)
{
  NQ_ASSERT(st);

  WCHAR winpath[MAX_PATH];
  if (NQWinPathFrom(winpath, sizeof(winpath), path) >= sizeof(winpath))
    return -ERROR_INVALID_PARAMETER;

  WIN32_FILE_ATTRIBUTE_DATA data;

  if (GetFileAttributesExW(winpath, GetFileExInfoStandard, &data) == 0)
      return -(int)GetLastError();

  st->mode = 0;
  if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    st->mode |= NQ_STAT_IFDIR;
  else
    st->mode |= NQ_STAT_IFREG;

  if (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
    st->mode |= NQ_STAT_IFLNK;

  st->size = ((uint64_t)data.nFileSizeHigh << 32) | (uint64_t)data.nFileSizeLow;
  st->accesseTimeMs = NQWinFileTimeToTimeMs(data.ftLastAccessTime.dwHighDateTime, data.ftLastAccessTime.dwLowDateTime);
  st->modificationTimeMs = NQWinFileTimeToTimeMs(data.ftLastWriteTime.dwHighDateTime, data.ftLastWriteTime.dwLowDateTime);
  st->creationTimeMs = NQWinFileTimeToTimeMs(data.ftCreationTime.dwHighDateTime, data.ftCreationTime.dwLowDateTime);

  return 0;
}
#endif
