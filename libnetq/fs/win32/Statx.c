/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/fs/Statx.h"

#ifdef NQ_OS_WINDOWS

#include <libnetq/ErrorCode.h>
#include <libnetq/fs/Path.h>
#include <libnetq/string/String.h>

#include <windows.h>

static void FromLargeInteger(LARGE_INTEGER t, NQTimeSpec* ts)
{
  if (t.QuadPart == 0) {
    ts->tv_sec = 0;
    ts->tv_nsec = 0;
    return;
  }
  uint64_t v = (uint64_t)t.QuadPart;
  NQWinFileTimeToTimeSpec((uint32_t)(v >> 32), (uint32_t)v, ts);
}

static void FromFileTime(FILETIME t, NQTimeSpec* ts)
{
  LARGE_INTEGER v;
  v.HighPart = (LONG)t.dwHighDateTime;
  v.LowPart = t.dwLowDateTime;
  FromLargeInteger(v, ts);
}

int NQGetStatx(const char* path, unsigned flags, NQStatx* st)
{
  NQMemset(st, 0, sizeof(*st));
  NQWinPath winpath;
  if (!NQWinPathInit(&winpath, path))
    return -NQ_ENAMETOOLONG;

  DWORD openFlags = FILE_FLAG_BACKUP_SEMANTICS | ((flags & NQ_STAT_FOLLOW) ? 0 : FILE_FLAG_OPEN_REPARSE_POINT);
  HANDLE handle = CreateFileW(winpath.characters, FILE_READ_ATTRIBUTES,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                              OPEN_EXISTING, openFlags, NULL);
  if (handle != INVALID_HANDLE_VALUE) {
    NQWinPathFinalize(&winpath);
    FILE_BASIC_INFO basic;
    BOOL ok = GetFileInformationByHandleEx(handle, FileBasicInfo, &basic, sizeof(basic));
    int result = ok ? 0 : -NQGetLastError();
    CloseHandle(handle);
    if (result != 0)
      return result;
    FromLargeInteger(basic.CreationTime, &st->btime);
    FromLargeInteger(basic.LastAccessTime, &st->atime);
    FromLargeInteger(basic.LastWriteTime, &st->mtime);
    FromLargeInteger(basic.ChangeTime, &st->ctime);
    st->result_mask = STATX_ATIME | STATX_MTIME | STATX_CTIME;
    /* zero when the file system keeps none */
    if (basic.CreationTime.QuadPart != 0)
      st->result_mask |= STATX_BTIME;
    return 0;
  }

  /* files that refuse even that (a page file in use): their attributes,
   * which describe a link itself */
  DWORD error = GetLastError();
  WIN32_FILE_ATTRIBUTE_DATA data;
  if ((error != ERROR_SHARING_VIOLATION && error != ERROR_ACCESS_DENIED) ||
      !GetFileAttributesExW(winpath.characters, GetFileExInfoStandard, &data)) {
    NQWinPathFinalize(&winpath);
    return -(int)error;
  }
  NQWinPathFinalize(&winpath);
  FromFileTime(data.ftCreationTime, &st->btime);
  FromFileTime(data.ftLastAccessTime, &st->atime);
  FromFileTime(data.ftLastWriteTime, &st->mtime);
  /* no change time here: ctime stays zero, without STATX_CTIME */
  st->result_mask = STATX_ATIME | STATX_MTIME;
  if (data.ftCreationTime.dwHighDateTime != 0 || data.ftCreationTime.dwLowDateTime != 0)
    st->result_mask |= STATX_BTIME;
  return 0;
}

#endif /* NQ_OS_WINDOWS */
