/*
 * MIT License
 *
 * Copyright (c) 2021-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/EventWakeup.h"

#if defined(HAVE_SYS_EVENTFD_H)
# include <unistd.h>
# include <sys/eventfd.h>
#elif defined(NQ_OS_WINDOWS) || defined(NQ_OS_UNIX)
# include <libnetq/net/SocketHandle.h>
# include <libnetq/Limits.h>
# define USE_SOCKETPAIR
#endif

#include <libnetq/Assert.h>

bool NQEventWakeup_init(NQEventWakeup* thiz)
{
#if defined(HAVE_SYS_EVENTFD_H)
  thiz->fd[0] = eventfd(0, EFD_NONBLOCK);
  thiz->fd[1] = -1;

  if (thiz->fd[0] == -1)
    return false;

  return true;
#elif defined(USE_SOCKETPAIR)
  NQSocketHandle socks[2];
  int status = NQSocketPair(NQ_AF_INET, NQ_SOCK_STREAM, NQ_IPPROTO_IP, socks);
  if (status != 0) {
    thiz->fd[0] = -1;
    thiz->fd[1] = -1;
    return false;
  }

  if (NQSocketSetNonBlocking(socks[0], true) || NQSocketSetNonBlocking(socks[1], true)) {
    NQSocketClose(socks[0]);
    NQSocketClose(socks[1]);
    thiz->fd[0] = -1;
    thiz->fd[1] = -1;
    return false;
  }

  NQ_ASSERT(socks[0] <= NQ_INT32_MAX);
  NQ_ASSERT(socks[1] <= NQ_INT32_MAX);

  thiz->fd[0] = (int)socks[0];
  thiz->fd[1] = (int)socks[1];

  return true;

#else
  return false;

#endif
}

void NQEventWakeup_finalize(NQEventWakeup* thiz)
{
#if defined(HAVE_SYS_EVENTFD_H)
  if (thiz->fd[0] != -1)
    close(thiz->fd[0]);

#elif defined(USE_SOCKETPAIR)
  if (thiz->fd[0] != -1) {
    NQSocketClose(thiz->fd[0]);
    thiz->fd[0] = -1;
  }
  if (thiz->fd[1] != -1) {
    NQSocketClose(thiz->fd[1]);
    thiz->fd[1] = -1;
  }
#endif
}

bool NQEventWakeup_sendUint64(NQEventWakeup* thiz, const uint64_t* value)
{
#if defined(HAVE_SYS_EVENTFD_H)
  ssize_t sz = write(thiz->fd[0], (uint8_t*)value, sizeof(*value));
  return (sz == sizeof(*value));

#elif defined(USE_SOCKETPAIR)
  size_t sz = NQSocketSend(thiz->fd[1], value, sizeof(*value), 0);
  return (sz == sizeof(*value));

#else
 return false;

#endif
}

bool NQEventWakeup_recvUint64(NQEventWakeup* thiz, uint64_t* value)
{
#if defined(HAVE_SYS_EVENTFD_H)
  ssize_t sz = read(thiz->fd[0], (uint8_t*)value, sizeof(*value));
  return (sz == sizeof(*value));

#elif defined(USE_SOCKETPAIR)
  size_t sz = NQSocketRecv(thiz->fd[0], (uint8_t*)value, sizeof(*value), 0);
  return (sz == sizeof(*value));

#else
 return false;

#endif
}
