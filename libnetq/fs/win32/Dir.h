/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_WIN32_DIR_H
#define _LIBNETQ_FS_WIN32_DIR_H

#include <libnetq/Basic.h>
#include <libnetq/fs/Path.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/Assert.h>

#include <windows.h>

struct NQDirImpl {
  HANDLE handle;
  WIN32_FIND_DATAW data;
  bool started;
  char name[MAX_PATH * 3 + 1];
};

static inline int NQDirImpl_open(struct NQDirImpl* thiz, const char* pathname)
{
  NQWinPath winpath;

  if (!NQWinPathInit(&winpath, pathname))
    return -NQ_ENAMETOOLONG;

  if (winpath.length + 3 > NQ_ARRAY_LENGTH(winpath.characters)) {
    NQWinPathFinalize(&winpath);
    return -NQ_ENAMETOOLONG;
  }
  if (winpath.characters[winpath.length - 1] != NQ_WIN32_PATH_SEPARATOR && winpath.characters[winpath.length - 1] != ':')
    winpath.characters[winpath.length++] = NQ_WIN32_PATH_SEPARATOR;
  winpath.characters[winpath.length++] = '*';
  winpath.characters[winpath.length] = '\0';

  thiz->handle = FindFirstFileW(winpath.characters, &thiz->data);
  if (thiz->handle == INVALID_HANDLE_VALUE) {
    int err = NQGetLastError();
    NQWinPathFinalize(&winpath);
    return err == ERROR_FILE_NOT_FOUND ? -NQ_ENOENT : (err ? -err : -NQ_EIO);
  }

  NQWinPathFinalize(&winpath);
  thiz->started = false;
  return 0;
}

static inline void NQDirImpl_close(struct NQDirImpl* thiz)
{
  FindClose(thiz->handle);
}

static inline bool NQDirImpl_first(struct NQDirImpl* thiz)
{
  if (thiz->started) {
    return false; // Not Supported
  }

  thiz->started = true;
  return NQPathFrom(thiz->name, sizeof(thiz->name), thiz->data.cFileName) > 0;
}

static inline bool NQDirImpl_next(struct NQDirImpl* thiz)
{
  NQ_ASSERT(thiz->started);

  if (!FindNextFileW(thiz->handle, &thiz->data))
    return false;

  return NQPathFrom(thiz->name, sizeof(thiz->name), thiz->data.cFileName) > 0;
}

static inline const char* NQDirImpl_name(const struct NQDirImpl* thiz)
{
  NQ_ASSERT(thiz->started);
  return thiz->started ? thiz->name : NULL;
}

static inline bool NQDirImpl_isLinkTag(DWORD attributes, DWORD tag)
{
  return (attributes & FILE_ATTRIBUTE_REPARSE_POINT) && (tag == IO_REPARSE_TAG_SYMLINK || tag == IO_REPARSE_TAG_MOUNT_POINT);
}

static inline bool NQDirImpl_isFile(const struct NQDirImpl* thiz)
{
  NQ_ASSERT(thiz->started);
  if (!thiz->started || NQDirImpl_isLinkTag(thiz->data.dwFileAttributes, thiz->data.dwReserved0))
    return false;
  return !(thiz->data.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE));
}

static inline bool NQDirImpl_isDirectory(const struct NQDirImpl* thiz)
{
  NQ_ASSERT(thiz->started);
  if (!thiz->started || NQDirImpl_isLinkTag(thiz->data.dwFileAttributes, thiz->data.dwReserved0))
    return false;
  return (thiz->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

#endif /* _LIBNETQ_FS_WIN32_DIR_H */
