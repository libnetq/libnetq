/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/fs/Statx.h"

#ifdef NQ_OS_UNIX

#include <libnetq/ErrorCode.h>

#include <errno.h>
#include <fcntl.h>
#if defined(NQ_OS_LINUX) && !defined(HAVE_STATX)
# include <sys/syscall.h>
# include <unistd.h>
#endif

int NQGetStatx(const char* path, unsigned flags, NQStatx* st)
{
#if defined(NQ_OS_LINUX) && defined(STATX_BTIME)
  int follow = (flags & NQ_STAT_FOLLOW) ? 0 : AT_SYMLINK_NOFOLLOW;
  unsigned mask = STATX_BASIC_STATS | STATX_BTIME;
# if defined(HAVE_STATX)
  int rc = statx(AT_FDCWD, path, follow, mask, st);
# elif defined(__NR_statx)
#  ifdef NQ_OS_ANDROID
  /* the seccomp filter of Android 10 and older kills a process making
   * the call (SIGSYS) rather than failing it */
  if (android_get_device_api_level() < 30)
    return -NQ_ENOTSUPP;
#  endif
  int rc = (int)syscall(__NR_statx, AT_FDCWD, path, follow, mask, st);
# else
  NQ_UNUSED_PARAM(follow);
  NQ_UNUSED_PARAM(mask);
  NQ_UNUSED_PARAM(path);
  NQ_UNUSED_PARAM(st);
  return -NQ_ENOTSUPP;
# endif
# if defined(HAVE_STATX) || defined(__NR_statx)
  if (rc != 0) {
    int error = errno;
    return error == ENOSYS ? -NQ_ENOTSUPP : -error;
  }
  return 0;
# endif
#elif defined(NQ_OS_DARWIN)
  if (((flags & NQ_STAT_FOLLOW) ? stat(path, st) : lstat(path, st)) != 0)
    return -NQGetLastError();
  return 0;
#else
  NQ_UNUSED_PARAM(path);
  NQ_UNUSED_PARAM(flags);
  NQ_UNUSED_PARAM(st);
  return -NQ_ENOTSUPP;
#endif
}

#endif /* NQ_OS_UNIX */
