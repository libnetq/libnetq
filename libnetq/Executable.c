/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/Executable.h"

#include <libnetq/ErrorCode.h>

#ifdef NQ_OS_UNIX
#include <unistd.h>
#include <libnetq/string/StringUtil.h>
#endif

#ifdef NQ_OS_DARWIN
#include <mach-o/dyld.h>
#endif

#ifdef NQ_OS_WINDOWS
#include <windows.h>
#include <libnetq/fs/Path.h>
#endif

int NQGetCommandLine(char* buffer, size_t n)
{
#ifdef NQ_OS_WINDOWS
  LPWSTR cmdline = GetCommandLineW();
  if (cmdline != NULL)
    return NQPathFrom(buffer, n, cmdline);
#endif

#ifdef NQ_OS_UNIX
  ssize_t i;
  ssize_t size = readlink("/proc/self/cmdline", buffer, n);
  if (size > 0) {
    size--;
    for (i = 0; i < size; i++) {
      if (buffer[i] == 0)
        buffer[i] = ' ';
    }
    return (size_t)size;
  }
#endif

  if (n != 0)
    *buffer = '\0';

  return 0;
}

int NQGetCurrentDirectory(char* buffer, size_t n)
{
#ifdef NQ_OS_WINDOWS
  WCHAR winpath[MAX_PATH];
  DWORD length = GetCurrentDirectoryW(MAX_PATH, winpath);
  if (length > 0)
    return NQPathFrom(buffer, n, winpath);
#endif

#ifdef NQ_OS_UNIX
  if (getcwd(buffer, n) != NULL)
    return NQStrlen(buffer);
#endif

  if (n != 0)
    *buffer = '\0';

  return 0;
}

int NQGetExecutablePath(char* buffer, size_t n)
{
#if defined(NQ_OS_WINDOWS)
  WCHAR winpath[MAX_PATH];
  DWORD length = GetModuleFileNameW(NULL, winpath, MAX_PATH);
  if (length >= MAX_PATH)
    return -NQ_ENAMETOOLONG;
  return (int)NQPathFrom(buffer, n, winpath);

#elif defined(NQ_OS_DARWIN)
  uint32_t iosize = (uint32_t)n;
  if (_NSGetExecutablePath(buffer, &iosize) == 0)
    return (int)iosize;
  return -NQ_ENAMETOOLONG;

#elif defined(NQ_OS_UNIX)
  // Linux:   /proc/self/exe
  // FreeBSD: /proc/curproc/file
  // Solaris: /proc/self/path/a.out
  ssize_t size = readlink("/proc/self/exe", buffer, n);
  if (size < 0)
    return -NQGetLastError();
  if ((size_t)size < n)
    buffer[size] = '\0';
  return (int)size;

#else
  return -NQ_ENOTSUPP;

#endif
}
