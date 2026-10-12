/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_KERNEL_DIR_H
#define _LIBNETQ_FS_KERNEL_DIR_H

#include <libnetq/Basic.h>
#include <libnetq/Assert.h>
#include <libnetq/ErrorCode.h>

#include <linux/namei.h>

struct NQDirImpl {
  struct path path;
  struct dentry* entry;
};

static inline int NQDirImpl_open(struct NQDirImpl* thiz, const char* pathname)
{
  int err;

  err = kern_path(pathname, LOOKUP_FOLLOW, &thiz->path);
  if (err != 0)
    return err;

  if (!d_is_dir(thiz->path.dentry)) {
    path_put(&thiz->path);
    return -NQ_ENOTDIR;
  }

  thiz->entry = NULL;
  return 0;
}

static inline void NQDirImpl_close(struct NQDirImpl* thiz)
{
  if (thiz->entry)
    dput(thiz->entry);
  path_put(&thiz->path);
}

static inline bool NQDirImpl_first(struct NQDirImpl* thiz)
{
  struct dentry* iter;

  if (thiz->entry) {
    dput(thiz->entry);
    thiz->entry = NULL;
  }

  spin_lock(&thiz->path.dentry->d_lock);
  iter = d_first_child(thiz->path.dentry);
  hlist_for_each_entry_from(iter, d_sib) {
    if (simple_positive(iter)) {
      thiz->entry = dget_dlock(iter);
      if (NQ_LIKELY(thiz->entry))
        break;
    }
  }
  spin_unlock(&thiz->path.dentry->d_lock);

  return thiz->entry != NULL;
}

static inline bool NQDirImpl_next(struct NQDirImpl* thiz)
{
  NQ_ASSERT(thiz->entry);

  struct dentry* iter;
  struct dentry* prev = thiz->entry;
  if (prev == NULL)
    return false;

  thiz->entry = NULL;

  spin_lock(&thiz->path.dentry->d_lock);
  iter = d_next_sibling(prev);
  hlist_for_each_entry_from(iter, d_sib) {
    if (simple_positive(iter)) {
      thiz->entry = dget_dlock(iter);
      if (NQ_LIKELY(thiz->entry))
        break;
    }
  }
  spin_unlock(&thiz->path.dentry->d_lock);

  dput(prev);
  return thiz->entry != NULL;
}

static inline const char* NQDirImpl_name(const struct NQDirImpl* thiz)
{
  NQ_ASSERT(thiz->entry);
  return thiz->entry ? (const char*)thiz->entry->d_name.name : NULL;
}

static inline bool NQDirImpl_isFile(const struct NQDirImpl* thiz)
{
  NQ_ASSERT(thiz->entry);
  return thiz->entry ? d_is_file(thiz->entry) : false;
}

static inline bool NQDirImpl_isDirectory(const struct NQDirImpl* thiz)
{
  NQ_ASSERT(thiz->entry);
  return thiz->entry ? d_is_dir(thiz->entry) : false;
}

#endif /* _LIBNETQ_FS_KERNEL_DIR_H */
