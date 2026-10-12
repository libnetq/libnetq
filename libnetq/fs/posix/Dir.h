/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_POSIX_DIR_H
#define _LIBNETQ_FS_POSIX_DIR_H

#include <libnetq/Basic.h>

#include <libnetq/ErrorCode.h>
#include <libnetq/Assert.h>

#include <dirent.h>

#ifdef NQ_OS_UNIX
#include <fcntl.h>
#include <sys/stat.h>
#endif

struct NQDirImpl {
  DIR* dir;
  struct dirent* entry;
  bool started;
};

static inline int NQDirImpl_open(struct NQDirImpl* thiz, const char* pathname)
{
  DIR* dir = opendir(pathname);
  if (dir == NULL) {
    int err = NQGetLastError();
    return err ? -err : -NQ_EIO;
  }

  thiz->dir = dir;
  thiz->entry = NULL;
  thiz->started = false;
  return 0;
}

static inline void NQDirImpl_close(struct NQDirImpl* thiz)
{
  closedir(thiz->dir);
}

static inline bool NQDirImpl_first(struct NQDirImpl* thiz)
{
  if (thiz->started)
    rewinddir(thiz->dir);
  else
    thiz->started = true;

  thiz->entry = readdir(thiz->dir);
  return thiz->entry != NULL;
}

static inline bool NQDirImpl_next(struct NQDirImpl* thiz)
{
  NQ_ASSERT(thiz->entry);
  if (thiz->entry == NULL)
    return false;

  thiz->entry = readdir(thiz->dir);
  return thiz->entry != NULL;
}

static inline const char* NQDirImpl_name(const struct NQDirImpl* thiz)
{
  NQ_ASSERT(thiz->entry);
  return thiz->entry ? thiz->entry->d_name : NULL;
}

static inline int NQDirImpl_type(const struct NQDirImpl* thiz)
{
#if defined(DT_UNKNOWN) && defined(NQ_OS_UNIX)
  if (thiz->entry->d_type == DT_UNKNOWN) {
    struct stat st;
    int fd = dirfd(thiz->dir);
    if (fstatat(fd, thiz->entry->d_name, &st, AT_SYMLINK_NOFOLLOW) == 0) {
      switch (st.st_mode & S_IFMT) {
      case S_IFREG:
        return DT_REG;
      case S_IFDIR:
        return DT_DIR;
      }
    }
  }
#endif

  return thiz->entry->d_type;
}

static inline bool NQDirImpl_isFile(const struct NQDirImpl* thiz)
{
  NQ_ASSERT(thiz->entry);
  return thiz->entry && NQDirImpl_type(thiz) == DT_REG;
}

static inline bool NQDirImpl_isDirectory(const struct NQDirImpl* thiz)
{
  NQ_ASSERT(thiz->entry);
  return thiz->entry && NQDirImpl_type(thiz) == DT_DIR;
}

#endif /* _LIBNETQ_FS_POSIX_DIR_H */
