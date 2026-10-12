/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/Looper.h"

#ifdef NQ_OS_DARWIN

#include <CoreFoundation/CoreFoundation.h>

#include <libnetq/Malloc.h>
#include <libnetq/Assert.h>
#include <libnetq/backend/cf/AutoRelease.h>

struct Looper {
  CFRunLoopRef nativeLooper;
};

static int Looper_init(NQLooper* looper)
{
  CFRunLoopRef nativeLooper = CFRunLoopGetCurrent();
  if (nativeLooper == NULL)
    return -1;

  struct Looper* thiz = NQMalloc(sizeof(struct Looper));
  if (thiz == NULL)
    return -1;

  CFRetain(nativeLooper);
  thiz->nativeLooper = nativeLooper;

  looper->priv = thiz;
  return 0;
}

static void Looper_finalize(NQLooper* looper)
{
  struct Looper* thiz = (struct Looper*)looper->priv;

  CFRelease(thiz->nativeLooper);

  NQFree(thiz);
  looper->priv = NULL;
}

static void Looper_dispatch(NQLooper* looper)
{
  void* pool = objc_autoreleasePoolPush();
  CFRunLoopRun();
  objc_autoreleasePoolPop(pool);
}

static void Looper_wake(NQLooper* looper)
{
  struct Looper* thiz = (struct Looper*)looper->priv;

  CFRunLoopWakeUp(thiz->nativeLooper);
}

static void Looper_stop(NQLooper* looper)
{
  struct Looper* thiz = (struct Looper*)looper->priv;

  NQ_ASSERT(thiz->nativeLooper == CFRunLoopGetCurrent());
  CFRunLoopStop(thiz->nativeLooper);
}

static bool Looper_attachSource(NQLooper* looper, NQLooperSource* source)
{
  return false;
}

static bool Looper_detachSource(NQLooper* looper, NQLooperSource* source)
{
  return false;
}

const struct NQLooperOperations CoreFoundationLooperOperations = {
  0,
  &Looper_init,
  &Looper_finalize,
  &Looper_dispatch,
  &Looper_wake,
  &Looper_stop,
  NULL, // poll
  &Looper_attachSource,
  &Looper_detachSource,
  NULL, // startTimer
  NULL, // stopTimer
  NULL, // getMessage
};

#endif
