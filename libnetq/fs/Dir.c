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

#include <libnetq/Malloc.h>
#include <libnetq/fs/Path.h>
#include <libnetq/Assert.h>

#if defined(NQ_OS_UNIX) || defined(NQCONFIG_USE_DIRENT_H)
#define USE_UNIX_DIR 1
#elif defined(NQ_OS_KERNEL)
#define USE_KERNEL_DIR 1
#elif defined(NQ_OS_WINDOWS)
#define USE_WINDOWS_DIR 1
#endif

#ifdef USE_KERNEL_DIR
#include <linux/namei.h>
#endif

#ifdef USE_UNIX_DIR
#include <dirent.h>
#endif

#ifdef USE_WINDOWS_DIR
#include <windows.h>
enum {
  NQ_DIR_OPEN_MODE,
  NQ_DIR_NEXT_MODE,
  NQ_DIR_EOF_MODE,
};
#endif

struct NQDir {

#ifdef USE_KERNEL_DIR
  struct path path;
  struct dentry* iter;
#endif

#ifdef USE_WINDOWS_DIR
  HANDLE handle;
  WIN32_FIND_DATAW data;
  uint8_t mode;
  char path[(MAX_PATH * 5) / 2]; /* 1/2 */
#endif

#ifdef USE_UNIX_DIR
  DIR* handle;
  struct dirent* dp;
#endif
};

NQDir* NQDir_open(const char* pathname)
{
  NQDir* dir;

#ifdef USE_KERNEL_DIR
  struct path path;
  struct dentry* first;
  struct dentry* iter;
  int ret;

  ret = kern_path(pathname, LOOKUP_FOLLOW, &path);
  if (ret != 0)
    return NULL;

  if (!d_is_dir(path.dentry)) {
    path_put(&path);
    return NULL;
  }

  spin_lock(&path.dentry->d_lock);
  iter = d_first_child(path.dentry);
  hlist_for_each_entry_from(iter, d_sib) {
    if (simple_positive(iter)) {
      first = dget_dlock(iter);
      if (NQ_LIKELY(first))
        break;
    }
  }
  spin_unlock(&path.dentry->d_lock);
  
  if (!first) {
    path_put(&path);
    return NULL;
  }

  dir = (NQDir*)NQMalloc(sizeof(NQDir));
  if (dir != NULL) {
    dir->path = path;
    dir->iter = first;
    return dir;
  }

  path_put(&path);
#endif

#ifdef USE_WINDOWS_DIR
  WCHAR winpath[MAX_PATH];
  size_t n = NQWinPathFrom(winpath, MAX_PATH, pathname);
  if (n == 0 || n > (MAX_PATH - 3))
    return NULL;

  if (winpath[n - 1] != NQ_WIN32_PATH_SEPARATOR)
    winpath[n++] = NQ_WIN32_PATH_SEPARATOR;
  winpath[n++] = '*';
  winpath[n++] = '\0';

  dir = (NQDir*)NQMalloc(sizeof(NQDir));
  if (dir == NULL)
    return NULL;

  dir->handle = FindFirstFileW(winpath, &dir->data);
  if (dir->handle != INVALID_HANDLE_VALUE) {
    dir->mode = NQ_DIR_OPEN_MODE;
    NQPathFrom((char*)dir->path, sizeof(dir->path), dir->data.cFileName);
    return dir;
  }

  NQFree(dir);
#endif

#ifdef USE_UNIX_DIR
  DIR* handle = opendir(pathname);
  if (handle == NULL)
    return NULL;

  struct dirent* dp = readdir(handle);
  if (dp != NULL) {
    dir = (NQDir*)NQMalloc(sizeof(NQDir));
    if (dir != NULL) {
      dir->handle = handle;
      dir->dp = dp;
      return dir;
    }
  }

  closedir(handle);
#endif

  return NULL;
}

bool NQDir_next(NQDir* dir)
{
#ifdef USE_KERNEL_DIR
  struct dentry* prev;
  struct dentry* iter;

  prev = dir->iter;
  if (prev == NULL)
    return false;

  spin_lock(&dir->path.dentry->d_lock);
  iter = d_next_sibling(dir->path.dentry);
  hlist_for_each_entry_from(iter, d_sib) {
    if (simple_positive(iter)) {
      dir->iter = dget_dlock(iter);
      if (NQ_LIKELY(dir->iter))
        break;
    }
  }
  spin_unlock(&dir->path.dentry->d_lock);
  dput(prev);

  if (dir->iter != NULL)
    return true;
#endif

#ifdef USE_WINDOWS_DIR
  if (dir->mode == NQ_DIR_EOF_MODE)
    return false;

  if (dir->mode == NQ_DIR_OPEN_MODE) {
    dir->mode = NQ_DIR_NEXT_MODE;
    return true;
  }

  if (FindNextFileW(dir->handle, &dir->data)) {
    NQPathFrom((char*)dir->path, sizeof(dir->path), dir->data.cFileName);
    return true;
  }
  dir->mode = NQ_DIR_EOF_MODE;
#endif

#ifdef USE_UNIX_DIR
  if (dir->dp == NULL)
    return false;

  dir->dp = readdir(dir->handle);
  if (dir->dp != NULL)
    return true;
#endif

  return false;
}

void NQDir_close(NQDir* dir)
{
#ifdef USE_KERNEL_DIR
  dput(dir->iter);
  path_put(&dir->path);
#endif

#ifdef USE_WINDOWS_DIR
  FindClose(dir->handle);
#endif

#ifdef USE_UNIX_DIR
  closedir(dir->handle);
#endif

  NQFree(dir);
}

const char* NQDir_name(NQDir* dir)
{
#ifdef USE_KERNEL_DIR
  if (dir->iter != NULL)
    return dir->iter->d_name.name;
#endif

#ifdef USE_WINDOWS_DIR
  if (dir->mode == NQ_DIR_NEXT_MODE)
    return dir->path;
#endif

#ifdef USE_UNIX_DIR
  if (dir->dp != NULL)
    return dir->dp->d_name;
#endif

  return NULL;
}

bool NQDir_isFile(NQDir* dir)
{
#ifdef USE_KERNEL_DIR
  if (dir->iter != NULL)
    return d_is_file(dir->iter);
#endif

#ifdef USE_WINDOWS_DIR
  if (dir->mode == NQ_DIR_NEXT_MODE)
    return dir->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? false : true;
#endif

#ifdef USE_UNIX_DIR
  if (dir->dp != NULL)
    return dir->dp->d_type == DT_REG;
#endif

  return false;
}

bool NQDir_isDirectory(NQDir* dir)
{
#ifdef USE_KERNEL_DIR
  if (dir->iter != NULL)
    return d_is_dir(dir->iter);
#endif

#ifdef USE_WINDOWS_DIR
  if (dir->mode == NQ_DIR_NEXT_MODE)
    return dir->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? true : false;
#endif

#ifdef USE_UNIX_DIR
  if (dir->dp != NULL)
    return dir->dp->d_type == DT_DIR;
#endif

  return false;
}
