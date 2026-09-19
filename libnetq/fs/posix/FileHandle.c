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

#ifdef NQ_OS_UNIX

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <libnetq/Limits.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/Assert.h>

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

  int ret = open(path, flags, 0666);
  if (ret == -1)
    return -NQGetLastError();

  *result = ret;
  return 0;
}

void NQFileClose(NQFileHandle handle)
{
  NQ_ASSERT(NQIsFileValid(handle));
  close(handle);
}

int NQFileRead(NQFileHandle handle, void* data, size_t size)
{
  int bytesRead;

  NQ_ASSERT(NQIsFileValid(handle));

  if (NQ_INT_MAX < size)
    return -1;

  do {
    bytesRead = read(handle, data, size);
    if (bytesRead >= 0)
      return bytesRead;
  } while (errno == EINTR);

  return -1;
}

int NQFileWrite(NQFileHandle handle, const void* data, size_t size)
{
  int bytesWritten;

  NQ_ASSERT(NQIsFileValid(handle));

  if (NQ_INT_MAX < size)
    return -1;

  do {
    bytesWritten = write(handle, data, size);
    if (bytesWritten >= 0)
      return bytesWritten;
  } while (errno == EINTR);

  return -1;
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
    return -NQ_EINVAL;
  }

  return (long long)lseek(handle, offset, whence);
}

long long NQFileGetSize(NQFileHandle handle)
{
  struct stat info;

  if (fstat(handle, &info) == -1)
    return -NQGetLastError();

  return info.st_size;
}

#endif /* NQ_OS_UNIX */
