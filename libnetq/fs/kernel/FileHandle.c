/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/fs/FileHandle.h"

#ifdef NQ_OS_KERNEL

#include <linux/fs.h>

#include <libnetq/Assert.h>
#include <libnetq/Log.h>
#include <libnetq/ErrorCode.h>

int NQFileOpen(const char* path, NQFileOpenMode mode, NQFileHandle* result)
{
  NQ_ASSERT(path && result);

  int flags = 0;
  switch (mode) {
  case NQ_FOPEN_READ:
    flags |= O_RDONLY;
    break;

  case NQ_FOPEN_WRITE:
    flags |= (O_WRONLY | O_CREAT | O_TRUNC);
    break;

  default:
    NQ_ASSERT_NOT_REACHED();
    return -NQ_EINVAL;
  }

  struct file* filp = filp_open(path, flags, 0666);
  if (IS_ERR(filp)) {
    long err = PTR_ERR(filp);
    NQ_LOGE("filp_open failed: %li", err);
    return (int)err;
  }

  *result = filp;
  return 0;
}

void NQFileClose(NQFileHandle handle)
{
    filp_close(handle, NULL);
}

int NQFileRead(NQFileHandle handle, void* data, size_t size)
{
  return kernel_read(handle, data, size, &handle->f_pos);
}

int NQFileWrite(NQFileHandle handle, const void* data, size_t size)
{
  return kernel_write(handle, data, size, &handle->f_pos);
}

long long NQFileSeek(NQFileHandle handle, long long offset, NQFileSeekOrigin origin)
{
  int whence;

  switch (origin) {
  case NQ_FSEEK_SET:
    whence = SEEK_SET;
    break;

  case NQ_FSEEK_CUR:
    whence = SEEK_CUR;
    break;

  case NQ_FSEEK_END:
    whence = SEEK_END;
    break;

  default:
    NQ_ASSERT_NOT_REACHED();
    return -1;
  }

  return (long long)vfs_llseek(handle, offset, whence);
}

long long NQFileGetSize(NQFileHandle handle)
{
  return (long long)i_size_read(file_inode(handle));
}

#endif /* NQ_OS_KERNEL */
