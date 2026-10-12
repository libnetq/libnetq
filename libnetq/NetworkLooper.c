/*
 * MIT License
 *
 * Copyright (c) 2025-2026 Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_CLASS_NAME "NQNetworkLooper"
#define NQ_LOG_TAG NQ_CLASS_NAME

#include "config.h"
#include "libnetq/NetworkLooper.h"

#include <libnetq/PlatformPoll.h>
#include <libnetq/EventWakeup.h>
#include <libnetq/Mutex.h>
#include <libnetq/Malloc.h>
#include <libnetq/List.h>
#include <libnetq/Limits.h>
#include <libnetq/Time.h>
#include <libnetq/Assert.h>
#include <libnetq/MinMax.h>

enum {
  kTimerInFreeState,
  kTimerInUsedState,
  kTimerInActionState,
  kTimerInDestroyState,

  kSocketInFreeState,
  kSocketInUsedState,
  kSocketInActionState,
  kSocketInDestroyState,

  kDispatchInFreeState,
  kDispatchInUsedState,
  kDispatchInActionState,
  kDispatchInDestroyState,
};

enum {
  kTimerActionEvent,
  kTimerDestroyEvent,
  kSocketActionEvent,
  kSocketDestroyEvent,
  kDispatchActionEvent,
  kDispatchDestroyEvent,
};

union EntryId {
  unsigned value;
  struct {
    unsigned index: 16;
    unsigned nonce: 16;
  };
};

struct BaseEntry {
  NQListHead list;
  union EntryId id;
  uint16_t state;
};

struct TimerEntry {
  struct BaseEntry base;

  int interval;
  int64_t actionTime;

  NQTimerActionHandler action;
  NQTimerDestroyHandler destroy;
  void* userdata;
};

struct SocketEntry {
  struct BaseEntry base;

  NQSocketActionHandler action;
  NQSocketDestroyHandler destroy;
  void* userdata;
};

struct DispatchEntry {
  struct BaseEntry base;

  NQDispatchActionHandler action;
  NQDispatchDestroyHandler destroy;
  void* userdata;
};

struct NQNetworkLooper {
  uint32_t timerLimit;
  uint32_t socketLimit;
  uint32_t dispatchLimit;

  NQEventWakeup wakeup;
  NQMutex mutex;

  int pollTimeout;
  int pollResult;

  NQListHead penndingList;

  NQListHead usedTimerList;
  NQListHead usedSocketList;

  NQListHead freeTimerList;
  NQListHead freeSocketList;
  NQListHead freeDispatchList;

  struct TimerEntry* timerEntries;
  struct SocketEntry* socketEntries;
  struct DispatchEntry* dispatchEntries;

  uint32_t pollSize;
  NQPlatformPollfd* pollfd;
};

static const uint64_t s_wakeupEvent = 0x8811AAFF;

bool NQNetworkLooper_wakeup(NQNetworkLooper* thiz)
{
  return NQEventWakeup_sendUint64(&thiz->wakeup, &s_wakeupEvent);
}

static int wakeupAction(NQSocketHandle handle, int events, void* userdata)
{
  NQNetworkLooper* thiz = (NQNetworkLooper*)userdata;
  if (events & NQ_POLLIN) {
    uint64_t event;
    NQEventWakeup_recvUint64(&thiz->wakeup, &event);
    NQ_ASSERT(event != 0 && (event % s_wakeupEvent) == 0);
  }

  return NQ_POLLIN;
}

#if NQ_DEBUG
static void prePollCheck(NQNetworkLooper* thiz)
{
  NQListHead* iter = thiz->usedSocketList.next;
  while (iter != &thiz->usedSocketList) {
    struct SocketEntry* entry = NQ_CONTAINER_OF(iter, struct SocketEntry, base.list);
    NQ_ASSERT(entry->base.id.index < thiz->pollSize);
    iter = iter->next;
  }
  for (uint32_t i = 0; i < thiz->pollSize; i++) {
    struct SocketEntry* entry = &thiz->socketEntries[i];
    NQ_ASSERT(entry->base.id.index == i);
  }
  if (!NQListHead_isEmpty(&thiz->freeSocketList)) {
    struct SocketEntry* entry = NQ_CONTAINER_OF(thiz->freeSocketList.next, struct SocketEntry, base.list);
    NQ_ASSERT(entry->base.id.index == thiz->pollSize);
  }
}
#else
static inline void prePollCheck(NQNetworkLooper* thiz) {}
#endif

static inline void initBaseEntry(struct BaseEntry* entry, uint16_t state, uint32_t index)
{
  NQListHead_init(&entry->list);
  entry->id.index = index;
  entry->id.nonce = 1;
  entry->state = state;
}

static inline void initTimerEntry(struct TimerEntry* entry, uint32_t index)
{
  initBaseEntry(&entry->base, kTimerInFreeState, index);

  entry->interval = 0;
  entry->actionTime = 0;
  entry->action = NULL;
  entry->destroy = NULL;
  entry->userdata = NULL;
}

static inline void initSocketEntry(struct SocketEntry* entry, uint32_t index)
{
  initBaseEntry(&entry->base, kSocketInFreeState, index);

  entry->action = NULL;
  entry->destroy = NULL;
  entry->userdata = NULL;
}

static inline void initDispatchEntry(struct DispatchEntry* entry, uint32_t index)
{
  initBaseEntry(&entry->base, kDispatchInFreeState, index);

  entry->action = NULL;
  entry->destroy = NULL;
  entry->userdata = NULL;
}

static inline void reinitBaseEntry(struct BaseEntry* entry, uint16_t state)
{
  entry->state = state;
  if (!++entry->id.nonce)
    entry->id.nonce++;
}

NQNetworkLooper* NQNetworkLooper_create(uint32_t timerLimit, uint32_t socketLimit, uint32_t dispatchLimit)
{
  if (NQ_UINT16_MAX < timerLimit || (NQ_UINT16_MAX - 1) < socketLimit || NQ_UINT16_MAX < dispatchLimit)
    return NULL;

  if (socketLimit == 0)
    socketLimit = 1;

  NQNetworkLooper* thiz;

  size_t timerSizeInBytes = sizeof(struct TimerEntry) * timerLimit;
  size_t socketSizeInBytes = sizeof(struct SocketEntry) * socketLimit;
  size_t dispatchSizeInBytes = sizeof(struct DispatchEntry) * dispatchLimit;

  size_t pollfdSizeInBytes = sizeof(NQPlatformPollfd) * socketLimit;

  size_t totalSizeInBytes = sizeof(*thiz) + timerSizeInBytes + socketSizeInBytes + dispatchSizeInBytes + pollfdSizeInBytes;
  thiz = (NQNetworkLooper*)NQMalloc(totalSizeInBytes);
  if (thiz == NULL)
    return NULL;

  if (!NQEventWakeup_init(&thiz->wakeup)) {
    NQFree(thiz);
    return NULL;
  }

  thiz->timerLimit = timerLimit;
  thiz->socketLimit = socketLimit;
  thiz->dispatchLimit = dispatchLimit;

  thiz->pollTimeout = 0;
  thiz->pollResult = -1;

  NQMutex_init(&thiz->mutex);

  NQListHead_init(&thiz->penndingList);

  NQListHead_init(&thiz->usedTimerList);
  NQListHead_init(&thiz->usedSocketList);

  NQListHead_init(&thiz->freeTimerList);
  NQListHead_init(&thiz->freeSocketList);
  NQListHead_init(&thiz->freeDispatchList);

  thiz->pollSize = 0;
  thiz->timerEntries = (struct TimerEntry*)((char*)thiz + sizeof(*thiz));
  thiz->socketEntries = (struct SocketEntry*)((char*)thiz->timerEntries + timerSizeInBytes);
  thiz->dispatchEntries = (struct DispatchEntry*)((char*)thiz->socketEntries + socketSizeInBytes);
  thiz->pollfd = (NQPlatformPollfd*)((char*)thiz->dispatchEntries + dispatchSizeInBytes);

  NQ_ASSERT(((char*)thiz->pollfd - (char*)thiz) + pollfdSizeInBytes == totalSizeInBytes);

  for (uint32_t i = 0; i < timerLimit; i++) {
    struct TimerEntry* entry = &thiz->timerEntries[i];
    initTimerEntry(entry, i);
    NQListHead_addBack(&thiz->freeTimerList, &entry->base.list);
  }

  for (uint32_t i = 0; i < socketLimit; i++) {
    struct SocketEntry* entry = &thiz->socketEntries[i];
    initSocketEntry(entry, i);
    NQListHead_addBack(&thiz->freeSocketList, &entry->base.list);
  }

  for (uint32_t i = 0; i < dispatchLimit; i++) {
    struct DispatchEntry* entry = &thiz->dispatchEntries[i];
    initDispatchEntry(entry, i);
    NQListHead_addBack(&thiz->freeDispatchList, &entry->base.list);
  }

#if defined(HAVE_SYS_EVENTFD_H) || defined(NQ_OS_WINDOWS) || defined(NQ_OS_UNIX)
  NQNetworkLooper_addSocket(thiz, NQEventWakeup_handle(&thiz->wakeup), wakeupAction, NULL, thiz);
#endif

  return thiz;
}

void NQNetworkLooper_destroy(NQNetworkLooper* thiz)
{
  NQListHead* iter;

  for (iter = thiz->penndingList.next; iter != &thiz->penndingList; iter = iter->next) {
    struct BaseEntry* iterEntry = NQ_CONTAINER_OF(thiz->penndingList.next, struct BaseEntry, list);
    switch (iterEntry->state) {
    case kTimerInActionState:
    case kTimerInDestroyState: {
      struct TimerEntry* entry = (struct TimerEntry*)iterEntry;
      if (entry->destroy)
        entry->destroy(entry->base.id.value, entry->userdata);
      break;
    }

    case kSocketInUsedState:
    case kSocketInActionState:
    case kSocketInDestroyState: {
      struct SocketEntry* entry = (struct SocketEntry*)iterEntry;
      if (entry->destroy) {
        uint32_t index = entry->base.id.index;
        NQPlatformPollfd* pollfd = &thiz->pollfd[index];
        entry->destroy(pollfd->fd, entry->userdata);
      }
      break;
    }

    case kDispatchInUsedState:
    case kDispatchInActionState:
    case kDispatchInDestroyState: {
      struct DispatchEntry* entry = (struct DispatchEntry*)iterEntry;
      if (entry->destroy)
        entry->destroy(entry->userdata);
      break;
    }

    default:
      NQ_ASSERT_NOT_REACHED();
      break;
    }
  }

  for (iter = thiz->usedTimerList.next; iter != &thiz->usedTimerList; iter = iter->next) {
    struct TimerEntry* entry = NQ_CONTAINER_OF(iter, struct TimerEntry, base.list);
    NQ_ASSERT(entry->base.state == kTimerInUsedState);
    if (entry->destroy)
      entry->destroy(entry->base.id.value, entry->userdata);
  }

  for (iter = thiz->usedSocketList.next; iter != &thiz->usedSocketList; iter = iter->next) {
    struct SocketEntry* entry = NQ_CONTAINER_OF(iter, struct SocketEntry, base.list);
    if (entry->base.state == kSocketInFreeState)
      continue;
    NQ_ASSERT(entry->base.state == kSocketInUsedState);
    if (entry->destroy) {
      uint32_t index = entry->base.id.index;
      NQPlatformPollfd* pollfd = &thiz->pollfd[index];
      entry->destroy(pollfd->fd, entry->userdata);
    }
  }

  NQMutex_finalize(&thiz->mutex);
  NQFree(thiz);
}

static void insertTimer(NQNetworkLooper* thiz, struct TimerEntry* entry)
{
  for (NQListHead* iter = thiz->usedTimerList.next; iter != &thiz->usedTimerList; iter = iter->next) {
    struct TimerEntry* iterEntry = NQ_CONTAINER_OF(iter, struct TimerEntry, base.list);
    if (entry->actionTime < iterEntry->actionTime) {
      NQListHead_addBack(&iterEntry->base.list, &entry->base.list);
      return;
    }
  }
  NQListHead_addBack(&thiz->usedTimerList, &entry->base.list);
}

static inline struct BaseEntry* nextEvent(NQNetworkLooper* thiz, int64_t now, int* event)
{
  if (NQListHead_isEmpty(&thiz->usedTimerList))
    thiz->pollTimeout = -1;
  else {
    struct TimerEntry* firstTimerEntry = NQ_CONTAINER_OF(thiz->usedTimerList.next, struct TimerEntry, base.list);
    thiz->pollTimeout = (int)(firstTimerEntry->actionTime - now);
    if (thiz->pollTimeout <= 0) {
      NQListHead_remove(&firstTimerEntry->base.list);
      firstTimerEntry->base.state = kTimerInActionState;
      NQListHead_addBack(&thiz->penndingList, &firstTimerEntry->base.list);
      *event = kTimerActionEvent;
      thiz->pollTimeout = 0;
      return &firstTimerEntry->base;
    }
  }

  while (!NQListHead_isEmpty(&thiz->penndingList)) {
    struct BaseEntry* baseEntry = NQ_CONTAINER_OF(thiz->penndingList.next, struct BaseEntry, list);
    NQListHead_remove(&baseEntry->list);

    switch (baseEntry->state) {
    case kTimerInActionState: {
      struct TimerEntry* entry = (struct TimerEntry*)baseEntry;
      if (entry->interval != 0) {
        entry->base.state = kTimerInUsedState;
        entry->actionTime += entry->interval;
        insertTimer(thiz, entry);
      }
      else if (entry->destroy == NULL) {
        entry->base.state = kTimerInFreeState;
        NQListHead_addBack(&thiz->freeTimerList, &baseEntry->list);
      }
      else {
        entry->base.state = kTimerInDestroyState;
        NQListHead_addBack(&thiz->penndingList, &baseEntry->list);
        *event = kTimerDestroyEvent;
        thiz->pollTimeout = 0;
        return baseEntry;
      }
      break;
    }

    case kTimerInDestroyState:
      baseEntry->state = kTimerInFreeState;
      NQListHead_addBack(&thiz->freeTimerList, &baseEntry->list);
      break;

    case kSocketInUsedState:
      baseEntry->state = kSocketInActionState;
      NQListHead_addBack(&thiz->penndingList, &baseEntry->list);
      *event = kSocketActionEvent;
      thiz->pollTimeout = 0;
      return baseEntry;

    case kSocketInActionState: {
      struct SocketEntry* entry = (struct SocketEntry*)baseEntry;
      uint32_t index = entry->base.id.index;
      NQPlatformPollfd* pollfd = &thiz->pollfd[index];
      if (pollfd->events != 0) {
        entry->base.state = kSocketInUsedState;
        NQListHead_addBack(&thiz->usedSocketList, &baseEntry->list);
      }
      else if (entry->destroy == NULL) {
        entry->base.state = kSocketInFreeState;
        NQListHead_addBack(&thiz->usedSocketList, &baseEntry->list);
      }
      else {
        entry->base.state = kSocketInDestroyState;
        NQListHead_addBack(&thiz->penndingList, &baseEntry->list);
      }
      break;
    }

    case kSocketInDestroyState: {
      baseEntry->state = kSocketInFreeState;
      NQListHead_addBack(&thiz->usedSocketList, &baseEntry->list);
      *event = kSocketDestroyEvent;
      thiz->pollTimeout = 0;
      return baseEntry;
    }

    case kDispatchInUsedState:
      baseEntry->state = kDispatchInActionState;
      NQListHead_addBack(&thiz->penndingList, &baseEntry->list);
      *event = kDispatchActionEvent;
      thiz->pollTimeout = 0;
      return baseEntry;

    case kDispatchInActionState: {
      struct DispatchEntry* entry = (struct DispatchEntry*)baseEntry;
      if (entry->destroy == NULL) {
        entry->base.state = kDispatchInFreeState;
        NQListHead_addBack(&thiz->freeDispatchList, &baseEntry->list);
      }
      else {
        entry->base.state = kDispatchInDestroyState;
        NQListHead_addBack(&thiz->penndingList, &baseEntry->list);
        *event = kDispatchDestroyEvent;
        thiz->pollTimeout = 0;
        return baseEntry;
      }
      break;
    }

    case kDispatchInDestroyState:
      baseEntry->state = kDispatchInFreeState;
      NQListHead_addBack(&thiz->freeDispatchList, &baseEntry->list);
      break;

    default:
      NQ_ASSERT_NOT_REACHED();
      break;
    }
  }

  NQListHead* iter = thiz->usedSocketList.next;
  while (iter != &thiz->usedSocketList) {
    NQListHead* next = iter->next;
    struct SocketEntry* entry = NQ_CONTAINER_OF(iter, struct SocketEntry, base.list);
    uint32_t index = entry->base.id.index;
    if (entry->base.state == kSocketInUsedState) {
      thiz->pollfd[index].revents = 0;
      NQListHead_remove(iter);
      NQListHead_addBack(&thiz->penndingList, iter);
    }
    else {
      NQ_ASSERT(entry->base.state == kSocketInFreeState);
      struct SocketEntry* lastSocketEntry = &thiz->socketEntries[--thiz->pollSize];
      if (entry != lastSocketEntry) {
        thiz->pollfd[index] = thiz->pollfd[thiz->pollSize];
        NQListHead_swap(&entry->base.list, &lastSocketEntry->base.list);
        entry->base.state = lastSocketEntry->base.state;
        entry->action = lastSocketEntry->action;
        entry->destroy = lastSocketEntry->destroy;
        entry->userdata = lastSocketEntry->userdata;
        lastSocketEntry->base.state = kSocketInFreeState;
      }
      next = lastSocketEntry->base.list.next;
      NQListHead_remove(&lastSocketEntry->base.list);
      NQListHead_addFront(&thiz->freeSocketList, &lastSocketEntry->base.list);
    }

    iter = next;
  }

  prePollCheck(thiz);
  return NULL;
}

int NQNetworkLooper_performOnce2(NQNetworkLooper* thiz, int64_t currentTimeMs)
{
  int event;
  struct BaseEntry* baseEntry;

  NQMutex_lock(&thiz->mutex);
  baseEntry = nextEvent(thiz, currentTimeMs, &event);
  NQMutex_unlock(&thiz->mutex);

  if (baseEntry == NULL) {
    return thiz->pollTimeout;
  }

  switch (event) {
  case kTimerActionEvent: {
    struct TimerEntry* timerEntry = (struct TimerEntry*)baseEntry;
    timerEntry->action(timerEntry->base.id.value, timerEntry->userdata);
    break;
  }
  case kTimerDestroyEvent: {
    struct TimerEntry* timerEntry = (struct TimerEntry*)baseEntry;
    timerEntry->destroy(timerEntry->base.id.value, timerEntry->userdata);
    break;
  }
  case kSocketActionEvent: {
    struct SocketEntry* socketEntry = (struct SocketEntry*)baseEntry;
    uint32_t index = socketEntry->base.id.index;
    NQPlatformPollfd* pollfd = &thiz->pollfd[index];
    pollfd->events = socketEntry->action(pollfd->fd, pollfd->revents, socketEntry->userdata);
    break;
  }
  case kSocketDestroyEvent: {
    struct SocketEntry* socketEntry = (struct SocketEntry*)baseEntry;
    uint32_t index = socketEntry->base.id.index;
    NQPlatformPollfd* pollfd = &thiz->pollfd[index];
    socketEntry->destroy(pollfd->fd, socketEntry->userdata);
    break;
  }
  case kDispatchActionEvent: {
    struct DispatchEntry* dispatchEntry = (struct DispatchEntry*)baseEntry;
    dispatchEntry->action(dispatchEntry->userdata);
    break;
  }
  case kDispatchDestroyEvent: {
    struct DispatchEntry* dispatchEntry = (struct DispatchEntry*)baseEntry;
    dispatchEntry->destroy(dispatchEntry->userdata);
    break;
  }
  default:
    NQ_ASSERT_NOT_REACHED();
    break;
  }

  return thiz->pollTimeout;
}

bool NQNetworkLooper_performOnceWithTime(NQNetworkLooper* thiz, int64_t currentTimeMs)
{
  return NQNetworkLooper_performOnce2(thiz, currentTimeMs) == 0;
}

bool NQNetworkLooper_performOnce(NQNetworkLooper* thiz)
{
  return NQNetworkLooper_performOnce2(thiz, NQGetCPUTickMs()) == 0;
}

int NQNetworkLooper_poll(NQNetworkLooper* thiz, int timeout)
{
  if (thiz->pollTimeout == 0)
    return thiz->pollResult;

  if (timeout == -1)
    timeout = thiz->pollTimeout;
  else if (thiz->pollTimeout != -1)
    timeout = NQGetMin(thiz->pollTimeout, timeout);

  thiz->pollResult = NQPlatformPoll(thiz->pollfd, thiz->pollSize, timeout);
  thiz->pollTimeout = 0;
  return thiz->pollResult;
}

bool NQNetworkLooper_runOnce(NQNetworkLooper* thiz)
{
  while (true) {
    int timeout = NQNetworkLooper_performOnce2(thiz, NQGetCPUTickMs());
    if (timeout == 0)
      return true;
    thiz->pollResult = NQPlatformPoll(thiz->pollfd, thiz->pollSize, timeout);
    if (thiz->pollResult < 0)
      break;
  }

  thiz->pollTimeout = 0;
  return false;
}

void NQNetworkLooper_removeAll(NQNetworkLooper* thiz)
{
  NQ_ASSERT_NOT_REACHED();
}

static uint32_t setTimer(NQNetworkLooper* thiz, int64_t actionTime, int interval, NQTimerActionHandler action, NQTimerDestroyHandler destroy, void* userdata)
{
  uint32_t result = 0;

  NQMutex_lock(&thiz->mutex);
  if (!NQListHead_isEmpty(&thiz->freeTimerList)) {
    struct TimerEntry* entry = NQ_CONTAINER_OF(thiz->freeTimerList.next, struct TimerEntry, base.list);
    NQ_ASSERT(entry->base.state == kTimerInFreeState);

    reinitBaseEntry(&entry->base, kTimerInUsedState);
    entry->interval = interval;
    entry->actionTime = actionTime;
    entry->action = action;
    entry->destroy = destroy;
    entry->userdata = userdata;

    result = entry->base.id.value;

    NQListHead_remove(&entry->base.list);
    insertTimer(thiz, entry);

    if (thiz->usedTimerList.next == &entry->base.list) {
      if (thiz->pollTimeout)
        NQNetworkLooper_wakeup(thiz);
    }
  }
  NQMutex_unlock(&thiz->mutex);

  return result;
}

static bool clearTimer(NQNetworkLooper* thiz, NQTimerIdentifier id, bool isInterval)
{
  // TODO: Sync if current entry
  bool result = true;

  union EntryId entryId;
  entryId.value = id;

  NQMutex_lock(&thiz->mutex);
  struct TimerEntry* entry = &thiz->timerEntries[entryId.index];
  if (entry->base.id.value != id || ((entry->interval != 0) != isInterval))
    result = false;
  else if (entry->base.state != kTimerInDestroyState) {
    NQ_ASSERT(entry->base.state == kTimerInUsedState || entry->base.state == kTimerInActionState);
    NQListHead_remove(&entry->base.list);
    if (entry->destroy == NULL) {
      entry->base.state = kTimerInFreeState;
      NQListHead_addBack(&thiz->freeTimerList, &entry->base.list);
    }
    else {
      entry->base.state = kTimerInDestroyState;
      NQListHead_addBack(&thiz->penndingList, &entry->base.list);
      if (thiz->pollTimeout)
        NQNetworkLooper_wakeup(thiz);
    }
  }
  NQMutex_unlock(&thiz->mutex);

  return result;
}

NQTimerIdentifier NQNetworkLooper_setTimeout(NQNetworkLooper* thiz, int delay, NQTimerActionHandler action, NQTimerDestroyHandler destroy, void* userdata)
{
  return setTimer(thiz, NQGetCPUTickMs() + delay, 0, action, destroy, userdata);
}

bool NQNetworkLooper_clearTimeout(NQNetworkLooper* thiz, NQTimerIdentifier id)
{
  return clearTimer(thiz, id, false);
}

NQTimerIdentifier NQNetworkLooper_setInterval(NQNetworkLooper* thiz, int delay, NQTimerActionHandler action, NQTimerDestroyHandler destroy, void* userdata)
{
  return setTimer(thiz, NQGetCPUTickMs() + delay, delay, action, destroy, userdata);
}

bool NQNetworkLooper_clearInterval(NQNetworkLooper* thiz, NQTimerIdentifier id)
{
  return clearTimer(thiz, id, true);
}

bool NQNetworkLooper_addSocket(NQNetworkLooper* thiz, NQSocketHandle handle, NQSocketActionHandler action, NQSocketDestroyHandler destroy, void* userdata)
{
  uint32_t result = 0;
  uint32_t index;

  NQMutex_lock(&thiz->mutex);
  if (!NQListHead_isEmpty(&thiz->freeSocketList)) {
    struct SocketEntry* entry = NQ_CONTAINER_OF(thiz->freeSocketList.next, struct SocketEntry, base.list);
    NQ_ASSERT(entry->base.state == kSocketInFreeState);

    reinitBaseEntry(&entry->base, kSocketInUsedState);
    entry->action = action;
    entry->destroy = destroy;
    entry->userdata = userdata;

    result = entry->base.id.value;

    NQ_ASSERT(entry == &thiz->socketEntries[thiz->pollSize]);
    index = thiz->pollSize++;
    NQ_ASSERT(index == entry->base.id.index);

    thiz->pollfd[index].fd = handle;
    thiz->pollfd[index].events = 0;
    thiz->pollfd[index].revents = 0;

    NQListHead_remove(&entry->base.list);
    NQListHead_addBack(&thiz->penndingList, &entry->base.list);

    if (thiz->pollTimeout)
      NQNetworkLooper_wakeup(thiz);
  }
  NQMutex_unlock(&thiz->mutex);

  return result;
}

bool NQNetworkLooper_removeSocket(NQNetworkLooper* thiz, NQSocketHandle handle)
{
  // TODO: Sync if current entry
  bool result = false;

  NQMutex_lock(&thiz->mutex);
  for (uint32_t i = 0; i < thiz->pollSize; i++) {
    if (thiz->pollfd[i].fd == handle) {
      struct SocketEntry* entry = &thiz->socketEntries[i];
      if (entry->base.state != kSocketInDestroyState && entry->base.state != kSocketInFreeState) {
        NQListHead_remove(&entry->base.list);
        if (entry->destroy == NULL) {
          entry->base.state = kSocketInFreeState;
          NQListHead_addBack(&thiz->usedSocketList, &entry->base.list);
        }
        else {
          entry->base.state = kSocketInDestroyState;
          NQListHead_addBack(&thiz->penndingList, &entry->base.list);
          if (thiz->pollTimeout)
            NQNetworkLooper_wakeup(thiz);
        }
      }
      result = true;
      break;
    }
  }
  NQMutex_unlock(&thiz->mutex);

  return result;
}

bool NQNetworkLooper_dispatch(NQNetworkLooper* thiz, NQDispatchActionHandler action, NQDispatchDestroyHandler destroy, void* userdata)
{
  uint32_t result = 0;

  NQMutex_lock(&thiz->mutex);
  if (!NQListHead_isEmpty(&thiz->freeDispatchList)) {
    struct DispatchEntry* entry = NQ_CONTAINER_OF(thiz->freeDispatchList.next, struct DispatchEntry, base.list);
    NQ_ASSERT(entry->base.state == kDispatchInFreeState);

    reinitBaseEntry(&entry->base, kDispatchInUsedState);
    entry->action = action;
    entry->destroy = destroy;
    entry->userdata = userdata;

    result = entry->base.id.value;

    NQListHead_remove(&entry->base.list);
    NQListHead_addBack(&thiz->penndingList, &entry->base.list);

    if (thiz->pollTimeout)
      NQNetworkLooper_wakeup(thiz);
  }
  NQMutex_unlock(&thiz->mutex);

  return result != 0;
}
