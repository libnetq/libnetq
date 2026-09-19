/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/fs/kernel/Link.h"

#ifdef NQ_OS_KERNEL

#include <libnetq/ErrorCode.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/mount.h>
#include <linux/fcntl.h>
#include <linux/version.h>

/*
 * security_path_symlink()/security_path_link() are not exported to modules,
 * so unlike NQMkdir() these skip the path-based LSM hooks. The inode-level
 * hooks are still invoked by vfs_symlink()/vfs_link().
 */

int NQSymlink(const char* target, const char* linkPath)
{
  unsigned int lookup_flags = 0;
  struct path parent;
  struct dentry *dentry;
  int err;

  if (!target || !*target || !linkPath || !*linkPath)
    return -NQ_EINVAL;

  might_sleep();

retry:
  dentry = start_creating_path(AT_FDCWD, linkPath, &parent, lookup_flags);
  if (IS_ERR(dentry))
    return PTR_ERR(dentry);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 19, 0)
  err = vfs_symlink(mnt_idmap(parent.mnt),
      d_inode(parent.dentry), dentry, target, NULL);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0)
  err = vfs_symlink(mnt_idmap(parent.mnt),
      d_inode(parent.dentry), dentry, target);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 12, 0)
  err = vfs_symlink(mnt_user_ns(parent.mnt),
      d_inode(parent.dentry), dentry, target);
#else
  err = vfs_symlink(d_inode(parent.dentry), dentry, target);
#endif

  end_creating_path(&parent, dentry);

  if (retry_estale(err, lookup_flags)) {
    lookup_flags |= LOOKUP_REVAL;
    goto retry;
  }

  return err;
}

int NQLink(const char* existingPath, const char* newPath)
{
  unsigned int lookup_flags = 0;
  struct path old_path;
  struct path parent;
  struct dentry *dentry;
  int err;

  if (!existingPath || !*existingPath || !newPath || !*newPath)
    return -NQ_EINVAL;

  might_sleep();

retry:
  /* Like link(2), do not follow a trailing symlink in existingPath. */
  err = kern_path(existingPath, lookup_flags, &old_path);
  if (err)
    return err;

  dentry = start_creating_path(AT_FDCWD, newPath, &parent, lookup_flags);
  if (IS_ERR(dentry)) {
    err = PTR_ERR(dentry);
    goto out_put_old;
  }

  err = -EXDEV;
  if (old_path.mnt != parent.mnt)
    goto out_done;

  /*
   * No delegated_inode: if an NFS delegation is held on the source,
   * vfs_link() fails with -EWOULDBLOCK instead of waiting for it.
   */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0)
  err = vfs_link(old_path.dentry, mnt_idmap(parent.mnt),
      d_inode(parent.dentry), dentry, NULL);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 12, 0)
  err = vfs_link(old_path.dentry, mnt_user_ns(parent.mnt),
      d_inode(parent.dentry), dentry, NULL);
#else
  err = vfs_link(old_path.dentry, d_inode(parent.dentry), dentry, NULL);
#endif

out_done:
  end_creating_path(&parent, dentry);
out_put_old:
  path_put(&old_path);

  if (retry_estale(err, lookup_flags)) {
    lookup_flags |= LOOKUP_REVAL;
    goto retry;
  }

  return err;
}

#endif
