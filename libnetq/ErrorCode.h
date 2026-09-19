/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_ERRORCODE_H
#define _LIBNETQ_ERRORCODE_H

#include <libnetq/Basic.h>

#if defined(NQ_OS_UNIX) || defined(NQCONFIG_USE_ERRNO_H)
# include <errno.h>
#elif defined(NQ_OS_KERNEL)
# include <linux/errno.h>
#elif defined(NQ_OS_WINDOWS)
# include <winerror.h>
#endif

typedef int NQErrorCode;

#if defined(NQ_OS_KERNEL) || defined(NQ_OS_UNIX) || defined(NQCONFIG_USE_ERRNO_H)
# define NQ_ENOENT           ENOENT
# define NQ_ENOMEM           ENOMEM
# define NQ_EBUSY            EBUSY
# define NQ_EINVAL           EINVAL
# define NQ_ENOSYS           ENOSYS
# define NQ_ETIMEDOUT        ETIMEDOUT
# define NQ_EINPROGRESS      EINPROGRESS
# define NQ_EWOULDBLOCK      EWOULDBLOCK
# define NQ_ERANGE           ERANGE
# define NQ_EIO              EIO
# define NQ_ENAMETOOLONG     ENAMETOOLONG
# define NQ_EOVERFLOW        EOVERFLOW
# define NQ_EACCES           EACCES
# define NQ_EISDIR           EISDIR

#if defined(NQ_OS_KERNEL)
# define NQ_ENOTSUPP         ENOTSUPP
# define NQ_ERESTARTSYS      ERESTARTSYS
#endif

#if defined(NQ_OS_UNIX) || defined(NQCONFIG_USE_ERRNO_H)
# define NQ_ENOTSUPP         ENOTSUP
# define NQ_ERESTARTSYS      EINTR
#endif

#elif defined(NQ_OS_WINDOWS)
# define NQ_ENOENT           ERROR_FILE_NOT_FOUND
# define NQ_ENOMEM           ERROR_NOT_ENOUGH_MEMORY
# define NQ_EBUSY            ERROR_BUSY
# define NQ_EINVAL           ERROR_INVALID_PARAMETER
# define NQ_ENOSYS           ERROR_CALL_NOT_IMPLEMENTED
# define NQ_ETIMEDOUT        ERROR_TIMEOUT
# define NQ_ENOTSUPP         ERROR_NOT_SUPPORTED
# define NQ_EINPROGRESS      WSAEINPROGRESS
# define NQ_EWOULDBLOCK      WSAEWOULDBLOCK
# define NQ_ERESTARTSYS      ERROR_OPERATION_ABORTED
# define NQ_ERANGE           ERROR_INSUFFICIENT_BUFFER
# define NQ_EIO              ERROR_IO_DEVICE
# define NQ_ENAMETOOLONG     ERROR_FILENAME_EXCED_RANGE
# define NQ_EOVERFLOW        ERROR_ARITHMETIC_OVERFLOW
# define NQ_EACCES           ERROR_ACCESS_DENIED
# define NQ_EISDIR           ERROR_ACCESS_DENIED

#endif

#if defined(NQ_OS_UNIX) || defined(NQCONFIG_USE_ERRNO_H)
static inline NQErrorCode NQGetLastError(void)
{
  return errno;
}
static inline void NQSetLastError(NQErrorCode ec)
{
  errno = ec;
}

#elif defined(NQ_OS_WINDOWS)
NQ_EXPORT NQErrorCode NQGetLastError(void);
NQ_EXPORT void NQSetLastError(NQErrorCode ec);

#else
static inline NQErrorCode NQGetLastError(void)
{
  return 0;
}
static inline void NQSetLastError(NQErrorCode ec)
{
  NQ_UNUSED_PARAM(ec);
}

#endif

static inline void NQClearLastError(void)
{
  NQSetLastError(0);
}

#endif /* _LIBNETQ_ERRORCODE_H */
