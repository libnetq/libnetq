/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_LOOPER_H
#define _LIBNETQ_LOOPER_H

#include <libnetq/Basic.h>

#include <libnetq/Mutex.h>
#include <libnetq/Thread.h>
#include <libnetq/Time.h>
#include <libnetq/DispatchQueue.h>
#include <libnetq/TimerQueue.h>
#include <libnetq/LooperTypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQLooper NQLooper;
typedef struct NQLooperSource NQLooperSource;

#define NQ_TIMER_ONCE (1 << 0)
#define NQ_TIMER_REPEAT (1 << 1)
#define NQ_TIMER_FIRED (1 << 2)
#define NQ_TIMER_RELEASE (1 << 3)

typedef void (*NQTimerCallback) (void* userdata, NQTimerIdentifier id, int flags);
typedef void (*NQDispatchHandleCallback) (void* userdata);
typedef void (*NQDispatchDestroyCallback) (void* userdata);

struct NQLooperOperations {
  int type;

  int  (*init)         (NQLooper* looper);
  void (*finalize)     (NQLooper* looper);

  void (*dispatch)     (NQLooper* looper);
  void (*wake)         (NQLooper* looper);
  void (*stop)         (NQLooper* looper);
  int  (*poll)         (NQLooper* looper, int64_t timeout);

  bool (*attachSource) (NQLooper* looper, NQLooperSource* source);
  bool (*detachSource) (NQLooper* looper, NQLooperSource* source);

  NQTimerIdentifier (*startTimer) (int64_t timeout, void* userdata, NQTimerCallback handle, int flags);
  void (*stopTimer) (NQTimerIdentifier id, int flags);

  bool (*getMessage)   (NQLooper* looper, NQMessage* message);
};

// Rename onto NQSystemLooper
struct NQLooper {
  NQMutex mutex;
  NQTID tid;

  bool stopLoop;
  bool notified;
  NQTimerQueue* timerQueue;
  NQDispatchQueue* dispatchQueue;
  NQTimeMs poolTime;

  void* priv;
  struct NQLooperOperations ops;
};

NQ_EXPORT void NQLooper_init(NQLooper* looper, const struct NQLooperOperations* ops);
NQ_EXPORT void NQLooper_finalize(NQLooper* looper);

NQ_EXPORT bool NQLooper_isLoopThread(const NQLooper* looper);
NQ_EXPORT void NQLooper_dispatch(NQLooper* looper);

NQ_EXPORT void NQLooper_call(NQLooper* looper, void* userdata, NQDispatchHandleCallback handle, NQDispatchDestroyCallback destroy);
NQ_EXPORT void NQLooper_callAndWait(NQLooper* looper, void* userdata, NQDispatchHandleCallback handle, NQDispatchDestroyCallback destroy);

NQ_EXPORT NQTimerIdentifier NQLooper_callTimeout(NQLooper* looper, int64_t timeout, void* userdata, NQDispatchHandleCallback handle, NQDispatchDestroyCallback destroy);
NQ_EXPORT void NQLooper_clearTimeout(NQLooper* looper, NQTimerIdentifier id);
NQ_EXPORT NQTimerIdentifier NQLooper_callInterval(NQLooper* looper, int64_t interval, void* userdata, NQDispatchHandleCallback handle, NQDispatchDestroyCallback destroy);
NQ_EXPORT void NQLooper_clearInterval(NQLooper* looper, NQTimerIdentifier id);

NQ_EXPORT void NQLooper_wakeup(NQLooper* looper);
NQ_EXPORT void NQLooper_stop(NQLooper* looper);

NQ_EXPORT bool NQLooper_attachSource(NQLooper* looper, NQLooperSource* source);
NQ_EXPORT bool NQLooper_detachSource(NQLooper* looper, NQLooperSource* source);

NQ_EXPORT void NQMainLooperInitialize(const struct NQLooperOperations* ops);
NQ_EXPORT void NQMainLooperShutdown(void);

NQ_EXPORT NQLooper* NQLooperGetMain(void);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_LOOPER_H */
