/*
 * MIT License
 *
 * Copyright (c) 2020-2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_POLL_H
#define _LIBNETQ_POLL_H

#include <libnetq/Basic.h>
#include <libnetq/net/SocketHandle.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_POLLIN      0x001
#define NQ_POLLPRI     0x002
#define NQ_POLLOUT     0x004

#define NQ_POLLRDNORM  0x040
#define NQ_POLLRDBAND  0x080
#define NQ_POLLWRNORM  0x100
#define NQ_POLLWRBAND  0x200

#define NQ_POLLERR     0x008
#define NQ_POLLHUP     0x010
#define NQ_POLLNVAL    0x020

typedef struct NQPollSocket {
  NQSocketHandle socket;
  uint16_t events;
  uint16_t revents;
} NQPollSocket;

NQ_EXPORT uint16_t NQPollEventsToPlatform(uint16_t events);
NQ_EXPORT uint16_t NQPollEventsFromPlatform(uint16_t events);

NQ_EXPORT int NQPollWait(NQPollSocket* data, size_t size, int64_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_POLL_H */
