/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_NETWORKLOOPER_H
#define _LIBNETQ_NETWORKLOOPER_H

#include <libnetq/net/SocketHandle.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQNetworkLooper NQNetworkLooper;

typedef uint32_t NQTimerIdentifier;
typedef void (*NQTimerActionHandler) (NQTimerIdentifier id, void* userdata);
typedef void (*NQTimerDestroyHandler) (NQTimerIdentifier id, void* userdata);

typedef int (*NQSocketActionHandler) (NQSocketHandle handle, int events, void* userdata);
typedef void (*NQSocketDestroyHandler) (NQSocketHandle handle, void* userdata);

typedef void (*NQDispatchActionHandler) (void* userdata);
typedef void (*NQDispatchDestroyHandler) (void* userdata);

NQ_EXPORT NQNetworkLooper* NQNetworkLooper_create(uint32_t timerLimit, uint32_t socketLimit, uint32_t dispatchLimit);
NQ_EXPORT void NQNetworkLooper_destroy(NQNetworkLooper*);

// Functions called only from a worker thread
NQ_EXPORT bool NQNetworkLooper_performOnce(NQNetworkLooper*);
NQ_EXPORT bool NQNetworkLooper_performOnceWithTime(NQNetworkLooper*, int64_t currentTimeMs);
NQ_EXPORT int NQNetworkLooper_performOnce2(NQNetworkLooper*, int64_t currentTimeMs);
NQ_EXPORT int NQNetworkLooper_poll(NQNetworkLooper*, int timeout);
NQ_EXPORT bool NQNetworkLooper_runOnce(NQNetworkLooper*);

NQ_EXPORT bool NQNetworkLooper_wakeup(NQNetworkLooper*);

NQ_EXPORT NQTimerIdentifier NQNetworkLooper_setTimeout(NQNetworkLooper*, int delay, NQTimerActionHandler action, NQTimerDestroyHandler destroy, void* userdata);
NQ_EXPORT bool NQNetworkLooper_clearTimeout(NQNetworkLooper*, NQTimerIdentifier id);
NQ_EXPORT NQTimerIdentifier NQNetworkLooper_setInterval(NQNetworkLooper*, int delay, NQTimerActionHandler action, NQTimerDestroyHandler destroy, void* userdata);
NQ_EXPORT bool NQNetworkLooper_clearInterval(NQNetworkLooper*, NQTimerIdentifier id);

NQ_EXPORT bool NQNetworkLooper_addSocket(NQNetworkLooper*, NQSocketHandle handle, NQSocketActionHandler action, NQSocketDestroyHandler destroy, void* userdata);
NQ_EXPORT bool NQNetworkLooper_removeSocket(NQNetworkLooper*, NQSocketHandle handle);

NQ_EXPORT bool NQNetworkLooper_dispatch(NQNetworkLooper*, NQDispatchActionHandler action, NQDispatchDestroyHandler destroy, void* userdata);

NQ_EXPORT void NQNetworkLooper_removeAll(NQNetworkLooper*);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_NETWORKLOOPER_H */
