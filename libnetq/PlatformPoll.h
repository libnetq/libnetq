/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_PLATFORMPOLL_H
#define _LIBNETQ_PLATFORMPOLL_H

#include <libnetq/ErrorCode.h>
#include <libnetq/net/SocketHandle.h>

#if defined(NQ_OS_KERNEL)
#include <linux/poll.h>
#elif defined(NQ_OS_UNIX)
#include <poll.h>
#elif defined(NQ_OS_WINDOWS)
#include <winsock2.h>
#endif

#if defined(NQ_OS_KERNEL) || defined(NQ_OS_UNIX) || defined(NQ_OS_WINDOWS)

#define NQ_POLLIN      POLLIN
#define NQ_POLLPRI     POLLPRI
#define NQ_POLLOUT     POLLOUT

#define NQ_POLLRDNORM  POLLRDNORM
#define NQ_POLLRDBAND  POLLRDBAND
#define NQ_POLLWRNORM  POLLWRNORM
#define NQ_POLLWRBAND  POLLWRBAND

#define NQ_POLLERR     POLLERR
#define NQ_POLLHUP     POLLHUP
#define NQ_POLLNVAL    POLLNVAL

#else

#define NQ_POLLIN      0x0001
#define NQ_POLLPRI     0x0002
#define NQ_POLLOUT     0x0004

#define NQ_POLLRDNORM  0x0040
#define NQ_POLLRDBAND  0x0080
#define NQ_POLLWRNORM  0x0100
#define NQ_POLLWRBAND  0x0200

#define NQ_POLLERR     0x0008
#define NQ_POLLHUP     0x0010
#define NQ_POLLNVAL    0x0020

#endif

#if defined(NQ_OS_UNIX)

typedef struct pollfd NQPlatformPollfd;

static inline int NQPlatformPoll(NQPlatformPollfd* fds, unsigned long nfds, int timeout)
{
  return poll(fds, nfds, timeout);
}

#elif defined(NQ_OS_WINDOWS)

typedef WSAPOLLFD NQPlatformPollfd;

static inline int NQPlatformPoll(NQPlatformPollfd* fds, unsigned long nfds, int timeout)
{
  return WSAPoll(fds, nfds, timeout);
}


#else

typedef struct NQPlatformPollfd NQPlatformPollfd;

struct NQPlatformPollfd {
  NQSocketHandle fd;
  short events;
  short revents;
};

static inline int NQPlatformPoll(NQPlatformPollfd* fds, unsigned long nfds, int timeout)
{
  NQ_UNUSED_PARAM(fds);
  NQ_UNUSED_PARAM(nfds);
  NQ_UNUSED_PARAM(timeout);

  return -NQ_ENOTSUPP;
}

#endif

#endif /* _LIBNETQ_PLATFORMPOLL_H */
