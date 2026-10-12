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
#include <libnetq/Malloc.h>
#include <libnetq/string/String.h>

#ifdef NQ_OS_WINDOWS
#include <windows.h>
#include <shlobj.h>
#endif

#if defined(NQ_OS_LINUX) && !defined(PATH_MAX)
#include <linux/limits.h>
#endif

#ifdef NQ_OS_UNIX
#include <fcntl.h> // for AT_FDCWD
#include <stdio.h> // for rename
#include <stdlib.h> // for realpath
#include <sys/stat.h>
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
  if (NQAbsoluteWinPath(winpath, MAX_PATH, path) < 0)
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
  NQWinPath winpath;
  if (!NQWinPathInit(&winpath, path))
    return -NQ_ENOMEM;
  BOOL success = DeleteFileW(winpath.characters);
  NQWinPathFinalize(&winpath);
  return success ? 0 : -NQGetLastError();
#else
  return -NQ_ENOTSUPP;
#endif
}

int NQRemoveDirectory(const char* path)
{
#if defined(NQ_OS_UNIX)
  if (rmdir(path) != 0)
    return -NQGetLastError();
  return 0;
#elif defined(NQ_OS_WINDOWS)
  NQWinPath winpath;
  if (!NQWinPathInit(&winpath, path))
    return -NQ_ENOMEM;
  BOOL success = RemoveDirectoryW(winpath.characters);
  NQWinPathFinalize(&winpath);
  return success ? 0 : -NQGetLastError();
#else
  return -NQ_ENOTSUPP;
#endif
}

#ifdef NQ_OS_WINDOWS
static bool IsWinDirectory(const NQWChar* path)
{
  DWORD attributes = GetFileAttributesW(path);
  return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}
#endif

int NQRename(const char* oldPath, const char* newPath)
{
#if defined(NQ_OS_UNIX)
  if (rename(oldPath, newPath) != 0)
    return -NQGetLastError();
  return 0;
#elif defined(NQ_OS_WINDOWS)
  NQWinPath winold, winnew;
  if (!NQWinPathInit(&winold, oldPath))
    return -NQ_ENOMEM;
  if (!NQWinPathInit(&winnew, newPath)) {
    NQWinPathFinalize(&winold);
    return -NQ_ENOMEM;
  }

  /* No MOVEFILE_COPY_ALLOWED: like rename(), fail across volumes. */
  BOOL success = MoveFileExW(winold.characters, winnew.characters, MOVEFILE_REPLACE_EXISTING);
  DWORD error = success ? 0 : GetLastError();

  /* MoveFileEx never replaces a directory; rename() replaces an empty one. */
  if (!success && (error == ERROR_ACCESS_DENIED || error == ERROR_ALREADY_EXISTS) &&
      IsWinDirectory(winold.characters) && IsWinDirectory(winnew.characters)) {
    if (RemoveDirectoryW(winnew.characters)) {
      success = MoveFileExW(winold.characters, winnew.characters, 0);
      error = success ? 0 : GetLastError();
    }
    else {
      error = GetLastError();
    }
  }

  NQWinPathFinalize(&winnew);
  NQWinPathFinalize(&winold);
  return success ? 0 : -(int)error;
#else
  return -NQ_ENOTSUPP;
#endif
}

int NQRealPath(char* buffer, size_t n, const char* path)
{
#if defined(NQ_OS_UNIX)
  char* resolved = realpath(path, NULL);
  if (!resolved)
    return -NQGetLastError();
  size_t length = NQStrlen(resolved);
  int result = (int)length;
  if (length >= n || length > NQ_INT32_MAX)
    result = -NQ_ERANGE;
  else
    NQMemcpy(buffer, resolved, length + 1);
  free(resolved);
  return result;
#elif defined(NQ_OS_WINDOWS)
  NQWinPath winpath;
  if (!NQWinPathInit(&winpath, path))
    return -NQ_ENOMEM;

  /* Opening the file follows the links on the way, unlike GetFullPathName. */
  HANDLE handle = CreateFileW(winpath.characters, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
  NQWinPathFinalize(&winpath);
  if (handle == INVALID_HANDLE_VALUE)
    return -NQGetLastError();

  WCHAR stack[MAX_PATH + 8];
  WCHAR* final = stack;
  DWORD length = GetFinalPathNameByHandleW(handle, final, NQ_ARRAY_LENGTH(stack), FILE_NAME_NORMALIZED);
  if (length >= NQ_ARRAY_LENGTH(stack)) {
    /* too small: the length returned counts the terminating null */
    final = NQMalloc(length * sizeof(WCHAR));
    length = final ? GetFinalPathNameByHandleW(handle, final, length, FILE_NAME_NORMALIZED) : 0;
  }
  int result = length == 0 ? (final ? -NQGetLastError() : -NQ_ENOMEM) : 0;
  CloseHandle(handle);

  if (result == 0) {
    /* "\\?\C:\dir" becomes "C:/dir", "\\?\UNC\host\share" becomes "//host/share" */
    WCHAR* start = final;
    if (length >= 8 && NQMemcmp(final, L"\\\\?\\UNC\\", 8 * sizeof(WCHAR)) == 0) {
      start = final + 6;
      start[0] = L'\\';
    }
    else if (length >= 4 && NQMemcmp(final, L"\\\\?\\", 4 * sizeof(WCHAR)) == 0) {
      start = final + 4;
    }

    /* at most three UTF-8 bytes per UTF-16 unit */
    size_t capacity = (size_t)length * 3 + 1;
    char* utf8 = NQMalloc(capacity);
    size_t written = utf8 ? NQPathFrom(utf8, capacity, (const NQWChar*)start) : 0;
    if (!utf8)
      result = -NQ_ENOMEM;
    else if (written == 0 || written > NQ_INT32_MAX)
      result = -NQ_EINVAL;
    else if (written >= n)
      result = -NQ_ERANGE;
    else {
      NQMemcpy(buffer, utf8, written + 1);
      result = (int)written;
    }
    NQFree(utf8);
  }

  if (final != stack)
    NQFree(final);
  return result;
#else
  NQ_UNUSED_PARAM(buffer);
  NQ_UNUSED_PARAM(n);
  NQ_UNUSED_PARAM(path);
  return -NQ_ENOTSUPP;
#endif
}

int NQSetFileMode(const char* path, unsigned mode)
{
#if defined(NQ_OS_UNIX)
  if (chmod(path, (mode_t)(mode & 07777)) != 0)
    return -NQGetLastError();
  return 0;
#elif defined(NQ_OS_WINDOWS)
  NQWinPath winpath;
  if (!NQWinPathInit(&winpath, path))
    return -NQ_ENOMEM;

  int result = 0;
  DWORD attributes = GetFileAttributesW(winpath.characters);
  if (attributes == INVALID_FILE_ATTRIBUTES)
    result = -NQGetLastError();
  else if (!(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
    DWORD wanted = (mode & 0200) ? (attributes & ~FILE_ATTRIBUTE_READONLY) : (attributes | FILE_ATTRIBUTE_READONLY);
    if (wanted == 0)
      wanted = FILE_ATTRIBUTE_NORMAL;
    if (wanted != attributes && !SetFileAttributesW(winpath.characters, wanted))
      result = -NQGetLastError();
  }

  NQWinPathFinalize(&winpath);
  return result;
#else
  NQ_UNUSED_PARAM(path);
  NQ_UNUSED_PARAM(mode);
  return -NQ_ENOTSUPP;
#endif
}

#if defined(NQ_OS_UNIX)
static void TimeMsToTimeSpecFloor(NQTimeMs time, struct timespec* ts)
{
  /* round towards the past, so times before 1970 keep 0 <= tv_nsec < 1s */
  int64_t sec = time / NQ_MSECS_PER_SEC;
  int64_t ms = time % NQ_MSECS_PER_SEC;
  if (ms < 0) {
    ms += NQ_MSECS_PER_SEC;
    --sec;
  }
  ts->tv_sec = (time_t)sec;
  ts->tv_nsec = (long)(ms * NQ_NSECS_PER_MSEC);
}
#elif defined(NQ_OS_WINDOWS)
static FILETIME TimeMsToWinFileTime(NQTimeMs time)
{
  uint64_t t = (uint64_t)(time * 10000 + (int64_t)116444736000000000LL);
  FILETIME ft;
  ft.dwLowDateTime = (DWORD)t;
  ft.dwHighDateTime = (DWORD)(t >> 32);
  return ft;
}
#endif

int NQSetFileTimes(const char* path, NQTimeMs accessTime, NQTimeMs modificationTime)
{
#if defined(NQ_OS_UNIX)
  struct timespec times[2];
  TimeMsToTimeSpecFloor(accessTime, &times[0]);
  TimeMsToTimeSpecFloor(modificationTime, &times[1]);
  if (utimensat(AT_FDCWD, path, times, 0) != 0)
    return -NQGetLastError();
  return 0;
#elif defined(NQ_OS_WINDOWS)
  NQWinPath winpath;
  if (!NQWinPathInit(&winpath, path))
    return -NQ_ENOMEM;

  /* FILE_FLAG_BACKUP_SEMANTICS opens directories as well */
  HANDLE handle = CreateFileW(winpath.characters, FILE_WRITE_ATTRIBUTES,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                              OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
  NQWinPathFinalize(&winpath);
  if (handle == INVALID_HANDLE_VALUE)
    return -NQGetLastError();

  FILETIME atime = TimeMsToWinFileTime(accessTime);
  FILETIME mtime = TimeMsToWinFileTime(modificationTime);
  BOOL success = SetFileTime(handle, NULL, &atime, &mtime);
  int result = success ? 0 : -NQGetLastError();
  CloseHandle(handle);
  return result;
#else
  NQ_UNUSED_PARAM(path);
  NQ_UNUSED_PARAM(accessTime);
  NQ_UNUSED_PARAM(modificationTime);
  return -NQ_ENOTSUPP;
#endif
}

#if defined(NQ_OS_WINDOWS)
static bool TimeSpecToWinFileTime(const NQTimeSpec* ts, FILETIME* ft)
{
  if (ts->tv_nsec == NQ_FILE_TIME_OMIT)
    return false;
  if (ts->tv_nsec == NQ_FILE_TIME_NOW) {
    GetSystemTimeAsFileTime(ft);
    return true;
  }
  uint64_t t = (uint64_t)((int64_t)ts->tv_sec * 10000000 + ts->tv_nsec / 100 + (int64_t)116444736000000000LL);
  ft->dwLowDateTime = (DWORD)t;
  ft->dwHighDateTime = (DWORD)(t >> 32);
  return true;
}
#endif

int NQSetFileTimeSpecs(const char* path, const NQTimeSpec times[2], bool followLinks)
{
#if defined(NQ_OS_UNIX)
  struct timespec ts[2];
  for (int i = 0; i < 2; ++i) {
    ts[i].tv_sec = times[i].tv_sec;
    ts[i].tv_nsec = times[i].tv_nsec == NQ_FILE_TIME_NOW ? UTIME_NOW
                  : times[i].tv_nsec == NQ_FILE_TIME_OMIT ? UTIME_OMIT
                  : times[i].tv_nsec;
  }
  if (utimensat(AT_FDCWD, path, ts, followLinks ? 0 : AT_SYMLINK_NOFOLLOW) != 0)
    return -NQGetLastError();
  return 0;
#elif defined(NQ_OS_WINDOWS)
  NQWinPath winpath;
  if (!NQWinPathInit(&winpath, path))
    return -NQ_ENOMEM;

  DWORD flags = FILE_FLAG_BACKUP_SEMANTICS | (followLinks ? 0 : FILE_FLAG_OPEN_REPARSE_POINT);
  HANDLE handle = CreateFileW(winpath.characters, FILE_WRITE_ATTRIBUTES,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                              OPEN_EXISTING, flags, NULL);
  NQWinPathFinalize(&winpath);
  if (handle == INVALID_HANDLE_VALUE)
    return -NQGetLastError();

  FILETIME atime, mtime;
  bool setA = TimeSpecToWinFileTime(&times[0], &atime);
  bool setM = TimeSpecToWinFileTime(&times[1], &mtime);
  BOOL success = SetFileTime(handle, NULL, setA ? &atime : NULL, setM ? &mtime : NULL);
  int result = success ? 0 : -NQGetLastError();
  CloseHandle(handle);
  return result;
#else
  NQ_UNUSED_PARAM(path);
  NQ_UNUSED_PARAM(times);
  NQ_UNUSED_PARAM(followLinks);
  return -NQ_ENOTSUPP;
#endif
}

unsigned NQGetUmask(void)
{
#if defined(NQ_OS_UNIX)
  /* POSIX reads the mask only by setting it */
  mode_t mask = umask(0);
  umask(mask);
  return (unsigned)mask;
#else
  return 0;
#endif
}

int NQCheckAccess(const char* path, int mode)
{
#if defined(NQ_OS_UNIX)
  int how = (mode & NQ_ACCESS_READ ? R_OK : 0) | (mode & NQ_ACCESS_WRITE ? W_OK : 0) |
            (mode & NQ_ACCESS_EXEC ? X_OK : 0);
  if (faccessat(AT_FDCWD, path, how ? how : F_OK, AT_EACCESS) != 0)
    return -NQGetLastError();
  return 0;
#elif defined(NQ_OS_WINDOWS)
  NQWinPath winpath;
  if (!NQWinPathInit(&winpath, path))
    return -NQ_ENOMEM;
  DWORD attributes = GetFileAttributesW(winpath.characters);
  int result = 0;
  if (attributes == INVALID_FILE_ATTRIBUTES)
    result = -NQGetLastError();
  /* directories may be read-only, which does not keep files out of them */
  else if ((mode & NQ_ACCESS_WRITE) && (attributes & FILE_ATTRIBUTE_READONLY) &&
           !(attributes & FILE_ATTRIBUTE_DIRECTORY))
    result = -NQ_EACCES;
  NQWinPathFinalize(&winpath);
  return result;
#else
  NQ_UNUSED_PARAM(path);
  NQ_UNUSED_PARAM(mode);
  return -NQ_ENOTSUPP;
#endif
}

int NQSetFileOwner(const char* path, unsigned owner, unsigned group, bool followLinks)
{
#if defined(NQ_OS_UNIX)
  if (fchownat(AT_FDCWD, path, (uid_t)owner, (gid_t)group, followLinks ? 0 : AT_SYMLINK_NOFOLLOW) != 0)
    return -NQGetLastError();
  return 0;
#else
  NQ_UNUSED_PARAM(path);
  NQ_UNUSED_PARAM(owner);
  NQ_UNUSED_PARAM(group);
  NQ_UNUSED_PARAM(followLinks);
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
