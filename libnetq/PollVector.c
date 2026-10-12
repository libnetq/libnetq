/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_CLASS_NAME "NQPollVector"
#define NQ_LOG_TAG NQ_CLASS_NAME

#include "config.h"
#include "libnetq/PollVector.h"

#include <libnetq/Limits.h>
#include <libnetq/Assert.h>
#include <libnetq/Malloc.h>
#include <libnetq/String.h>
#include <libnetq/MinMax.h>

#define MIN_CAPACITY (16)

void NQPollVector_init(NQPollVector* thiz)
{
  thiz->size = 0;
  thiz->capacity = 0;
  thiz->data = NULL;
  thiz->pollfd = NULL;
}

void NQPollVector_finalize(NQPollVector* thiz)
{
  if (thiz->data)
    NQFree(thiz->data);
}

static bool NQPollVector_reserveCapacity(NQPollVector* thiz, size_t newCapacity)
{
  if (NQ_UINT16_MAX < newCapacity)
    return false;

  if (newCapacity <= thiz->capacity)
    return true;

  struct NQPollData* newData = (struct NQPollData*)NQMalloc(newCapacity * (sizeof(struct NQPollData) + sizeof(NQPlatformPollfd)));
  if (newData == NULL)
    return false;

  NQPlatformPollfd* newPollfd = (NQPlatformPollfd*)((char*)newData + newCapacity * (sizeof(struct NQPollData)));
  if (thiz->size != 0) {
    NQMemcpy(newData, thiz->data, thiz->size);
    NQMemcpy(newPollfd, thiz->pollfd, thiz->size);
    NQFree(thiz->data);
  }

  thiz->data = newData;
  thiz->pollfd = newPollfd;
  thiz->capacity = (uint16_t)newCapacity;

  return true;
}

static inline uint16_t NQPollVector_nextCapacity(NQPollVector* thiz, uint16_t minCapacity)
{
  uint16_t result;
  if (thiz->capacity == 0)
    result = MIN_CAPACITY;
  else
    result = NQGetMin(thiz->capacity + thiz->capacity / 2, NQ_UINT16_MAX);
  return NQGetMax(minCapacity, result);
}

static inline bool NQPollVector_expandCapacity(NQPollVector* thiz, uint16_t minCapacity)
{
  return NQPollVector_reserveCapacity(thiz, NQPollVector_nextCapacity(thiz, minCapacity));
}

int NQPollVector_poll(NQPollVector* thiz, int timeout)
{
  return NQPlatformPoll(thiz->pollfd, thiz->size, timeout);
}

void NQPollVector_action(NQPollVector* thiz)
{
  for (uint16_t i = 0; i < thiz->size; i++) {
    if (thiz->data[i].action == NULL)
      continue;

    NQSocketHandle handler = thiz->pollfd[i].fd;
    short revents = thiz->pollfd[i].revents;

    NQPollActionHandler action = thiz->data[i].action;
    void* userdata = thiz->data[i].userdata;

    /*
     * After calling the handler for action and destroy,
     * the array can change its size and reallocate itself,
     * that's why the link iterator is not used
     *
     */

    int events = action(handler, revents, userdata);
    if (events == 0)
      thiz->data[i].action = NULL;
    else {
      thiz->pollfd[i].events = events;
      thiz->pollfd[i].revents = 0;
    }
  }

  uint16_t fdGapSize = 0;
  for (uint16_t i = 0; i < thiz->size; i++) {
    if (thiz->data[i].action == NULL) {
      if (thiz->data[i].destroy)
        thiz->data[i].destroy(thiz->pollfd[i].fd, thiz->data[i].userdata);
      fdGapSize++;
    }
    else if (fdGapSize != 0) {
      thiz->pollfd[i - fdGapSize] = thiz->pollfd[i];
      thiz->data[i - fdGapSize] = thiz->data[i];
    }
  }
  thiz->size -= fdGapSize;
}

bool NQPollVector_add(NQPollVector* thiz, NQSocketHandle handle, NQPollActionHandler action, NQPollDestroyHandler destroy, void* userdata)
{
  if (action == NULL)
    return false;

  if (thiz->size == NQ_UINT16_MAX)
    return false;

  uint16_t newSize = thiz->size + 1;
  if (newSize > thiz->capacity) {
    if (!NQPollVector_expandCapacity(thiz, newSize))
      return false;
  }

  thiz->data[thiz->size].userdata = userdata;
  thiz->data[thiz->size].action = action;
  thiz->data[thiz->size].destroy = destroy;

  thiz->pollfd[thiz->size].fd = handle;
  thiz->pollfd[thiz->size].events = 0;
  thiz->pollfd[thiz->size].revents = 0;

  thiz->size = newSize;

  return true;
}

bool NQPollVector_remove(NQPollVector* thiz, NQSocketHandle handle)
{
  for (uint16_t i = 0; i < thiz->size; i++) {
    if (thiz->pollfd[i].fd == handle) {
      thiz->data[i].action = NULL;
      return true;
    }
  }

  return false;
}

void NQPollVector_removeAll(NQPollVector* thiz)
{
  for (uint16_t i = 0; i < thiz->size; i++)
    thiz->data[i].action = NULL;
  thiz->size = 0;
}
