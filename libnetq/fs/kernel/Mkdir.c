/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/fs/kernel/Mkdir.h"

#ifdef NQ_OS_KERNEL

#include <libnetq/ErrorCode.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/mount.h>
#include <linux/security.h>
#include <linux/fcntl.h>
#include <linux/version.h>

int NQMkdir(const char* path, mode_t mode)
{
  unsigned int lookup_flags = LOOKUP_DIRECTORY;
  struct path parent;
  struct dentry *dentry;
  umode_t m = (umode_t)mode & (S_IRWXUGO | S_ISVTX);
  int err;

  if (!path || !*path)
    return -NQ_EINVAL;

  might_sleep();

retry:
  /*
   * Looks up the parent, locks it, takes write access on the mount
   * and returns a negative dentry for the new name.
   * Returns -EEXIST if the name already exists.
   */
  dentry = start_creating_path(AT_FDCWD, path, &parent, lookup_flags);
  if (IS_ERR(dentry))
    return PTR_ERR(dentry);

  err = security_path_mkdir(&parent, dentry, m);
  if (!err) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 19, 0)
    dentry = vfs_mkdir(mnt_idmap(parent.mnt),
           d_inode(parent.dentry), dentry, m, NULL);
    if (IS_ERR(dentry))
      err = PTR_ERR(dentry);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6, 15, 0)
    dentry = vfs_mkdir(mnt_idmap(parent.mnt),
           d_inode(parent.dentry), dentry, m);
    if (IS_ERR(dentry))
      err = PTR_ERR(dentry);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0)
    err = vfs_mkdir(mnt_idmap(parent.mnt),
        d_inode(parent.dentry), dentry, m);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 12, 0)
    err = vfs_mkdir(mnt_user_ns(parent.mnt),
        d_inode(parent.dentry), dentry, m);
#else
    err = vfs_mkdir(d_inode(parent.dentry), dentry, m);
#endif
  }

  /* Unlocks the parent, drops mount write access and both references. */
  end_creating_path(&parent, dentry);

  /* Stale NFS handle: retry once with revalidation, as do_mkdirat() does. */
  if (retry_estale(err, lookup_flags)) {
    lookup_flags |= LOOKUP_REVAL;
    goto retry;
  }

  return err;
}

#endif
