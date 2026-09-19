/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_CLASS_NAME "NQFileSystem"
#define NQ_LOG_TAG NQ_CLASS_NAME

#include "config.h"
#include "libnetq/fs/FileSystem.h"

#include <libnetq/fs/FileHandle.h>
#include <libnetq/fs/Path.h>
#include <libnetq/Limits.h>
#include <libnetq/Log.h>
#include <libnetq/fs/Stat.h>
#include <libnetq/ErrorCode.h>

#ifdef NQ_OS_WINDOWS
#include <windows.h>
#include <shlobj.h>
#endif

#if defined(NQ_OS_LINUX) && !defined(PATH_MAX)
#include <linux/limits.h>
#endif

#ifdef NQ_OS_UNIX
#include <unistd.h>
#endif

int64_t NQReadFile(const char* path, uint8_t* data, int64_t size)
{
  int64_t result;
  NQFileHandle handle;

  result = NQFileOpen(path, NQ_FOPEN_READ, &handle);
  if (result != 0)
    return result;

  result = NQFileReadn(handle, data, size);
  NQFileClose(handle);

  return result;
}

int64_t NQWriteFile(const char* path, const uint8_t* data, int64_t size)
{
  int64_t result;
  NQFileHandle handle;

  result = NQFileOpen(path, NQ_FOPEN_WRITE, &handle);
  if (result != 0)
    return result;

  result = NQFileWriten(handle, data, size);
  NQFileClose(handle);

  return result;
}

bool NQMakeDirectory(const char* path, bool recursive)
{
  (void)recursive; // TODO

#ifdef NQ_OS_WINDOWS
  WCHAR winpath[MAX_PATH];
  if (NQGetAbsoluteWinPath(winpath, MAX_PATH, path) == 0)
    return false;
  if (SHCreateDirectoryExW(NULL, winpath, NULL) == ERROR_SUCCESS)
    return true;
  DWORD error = GetLastError();
  if (error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS)
    return true;

#endif

#ifdef NQ_OS_UNIX
  if (!access(path, F_OK))
    return true;

  char fullPath[PATH_MAX];
  size_t length = NQGetAbsolutePath(fullPath, sizeof(fullPath), path);
  if (length == 0 || sizeof(fullPath) <= length)
    return false;

  char* p = fullPath + 1;
  if (p[length - 1] == '/')
    p[length - 1] = '\0';
  for (; *p; ++p) {
    if (*p == '/') {
      *p = '\0';
      if (access(fullPath, F_OK)) {
        if (mkdir(fullPath, S_IRWXU))
          return false;
      }
      *p = '/';
    }
  }
  if (!access(fullPath, F_OK))
    return true;
  if (!mkdir(fullPath, S_IRWXU))
    return true;

#endif

  return false;
}

bool NQFileExists(const char* path)
{
  NQStat stat;
  int err = NQGetStat(path, &stat);
  if (err)
    return false;
  return NQStat_isFile(&stat);
}

bool NQDirectoryExists(const char* path)
{
  NQStat stat;
  int err = NQGetStat(path, &stat);
  if (err)
    return false;
  return NQStat_isDirectory(&stat);
}

int NQRemoveFile(const char* path)
{
#if defined(NQ_OS_UNIX)
  if (unlink(path) != 0)
    return -NQGetLastError();
  return 0;
#elif defined(NQ_OS_WINDOWS)
  WCHAR winpath[MAX_PATH];
  NQWinPathFrom(winpath, MAX_PATH, path);
  if (!DeleteFileW(winpath))
    return -NQGetLastError();
  return 0;
#else
  return -NQ_ENOTSUPP;
#endif
}

int NQGetLogicalDrives(void)
{
#ifdef NQ_OS_WINDOWS
  DWORD ret = GetLogicalDrives();
  if (ret == 0)
    return -NQGetLastError();
  return ret;
#else
  return 0;
#endif
}
