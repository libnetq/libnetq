/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_POLLVECTOR_H
#define _LIBNETQ_POLLVECTOR_H

#include <libnetq/net/SocketHandle.h>
#include <libnetq/PlatformPoll.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQPollVector NQPollVector;

typedef int (*NQPollActionHandler) (NQSocketHandle handle, int events, void* userdata);
typedef void (*NQPollDestroyHandler) (NQSocketHandle handle, void* userdata);

struct NQPollData {
  void* userdata;
  NQPollActionHandler action;
  NQPollDestroyHandler destroy;
};

struct NQPollVector {
  uint16_t size;
  uint16_t capacity;
  struct NQPollData* data;
  NQPlatformPollfd* pollfd;
};

NQ_EXPORT void NQPollVector_init(NQPollVector*);
NQ_EXPORT void NQPollVector_finalize(NQPollVector*);
NQ_EXPORT int NQPollVector_poll(NQPollVector*, int timeout);
NQ_EXPORT void NQPollVector_action(NQPollVector*);
NQ_EXPORT bool NQPollVector_add(NQPollVector*, NQSocketHandle handle, NQPollActionHandler action, NQPollDestroyHandler destroy, void* userdata);
NQ_EXPORT bool NQPollVector_remove(NQPollVector*, NQSocketHandle handle);
NQ_EXPORT void NQPollVector_removeAll(NQPollVector*);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_POLLVECTOR_H */
