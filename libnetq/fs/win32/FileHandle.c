/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/fs/FileHandle.h"

#ifdef NQ_OS_WINDOWS

#include <libnetq/fs/Path.h>
#include <libnetq/Limits.h>
#include <libnetq/Log.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/Assert.h>

static HANDLE NQFileOpenImpl(const char* path, DWORD dwDesiredAccess, DWORD dwShareMode,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
  DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
  WCHAR winpath[MAX_PATH];
  if (NQWinPathFrom(winpath, MAX_PATH, path) > 0)
    return CreateFileW(winpath, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
  return CreateFileA(path, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

int NQFileOpen(const char* path, NQFileOpenMode mode, NQFileHandle* result)
{
  NQ_ASSERT(path && result);

  DWORD desiredAccess;
  DWORD creationDisposition;
  DWORD shareMode;

  switch (mode) {
  case NQ_FOPEN_READ:
    desiredAccess = GENERIC_READ;
    creationDisposition = OPEN_EXISTING;
    shareMode = FILE_SHARE_READ;
    break;

  case NQ_FOPEN_WRITE:
    desiredAccess = GENERIC_WRITE;
    creationDisposition = CREATE_ALWAYS;
    shareMode = 0;
    break;

  default:
    NQ_ASSERT_NOT_REACHED();
    return -NQ_EINVAL;
  }

  NQFileHandle handle = NQFileOpenImpl(path, desiredAccess, shareMode, 0, creationDisposition, FILE_ATTRIBUTE_NORMAL, 0);
  if (handle == INVALID_HANDLE_VALUE) {
    int ret = GetLastError();
    NQ_LOGE("CreateFile returned %i", -ret);
    return -ret;
  }

  *result = handle;
  return 0;
}

void NQFileClose(NQFileHandle handle)
{
  NQ_ASSERT(NQIsFileValid(handle));
  CloseHandle(handle);
}

int NQFileRead(NQFileHandle handle, void* data, size_t size)
{
  DWORD bytesRead;

  NQ_ASSERT(NQIsFileValid(handle));

  if (NQ_INT_MAX < size)
    return -1;

  if (!ReadFile(handle, data, (DWORD)size, &bytesRead, 0))
    return -1;

  return (int)bytesRead;
}

int NQFileWrite(NQFileHandle handle, const void* data, size_t size)
{
  DWORD bytesWritten;

  NQ_ASSERT(NQIsFileValid(handle));

  if (NQ_INT_MAX < size)
    return -1;

  if (!WriteFile(handle, data, (DWORD)size, &bytesWritten, 0))
    return -1;

  return (int)bytesWritten;
}

long long NQFileSeek(NQFileHandle handle, long long offset, NQFileSeekOrigin origin)
{
  DWORD moveMethod;
  LARGE_INTEGER largeOffset;

  NQ_ASSERT(NQIsFileValid(handle));

  switch (origin) {
  case NQ_FSEEK_SET:
    moveMethod = FILE_BEGIN;
    break;

  case NQ_FSEEK_CUR:
    moveMethod = FILE_CURRENT;
    break;

  case NQ_FSEEK_END:
    moveMethod = FILE_END;
    break;

  default:
    NQ_ASSERT_NOT_REACHED();
    return -1;
  }

  largeOffset.QuadPart = offset;
  largeOffset.LowPart = SetFilePointer(handle, largeOffset.LowPart, &largeOffset.HighPart, moveMethod);

  if (largeOffset.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
    return -1;

  return largeOffset.QuadPart;
}

long long NQFileGetSize(NQFileHandle handle)
{
  ULARGE_INTEGER fileSize;
  BY_HANDLE_FILE_INFORMATION info;

  if (!GetFileInformationByHandle(handle, &info))
    return -NQGetLastError();

  fileSize.HighPart = info.nFileSizeHigh;
  fileSize.LowPart = info.nFileSizeLow;

  if (fileSize.QuadPart > NQ_LLONG_MAX)
    return -NQ_EOVERFLOW;

  return (long long)fileSize.QuadPart;
}

#endif /* NQ_OS_WINDOWS */
