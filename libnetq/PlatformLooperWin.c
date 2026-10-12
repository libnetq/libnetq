/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/PlatformLooper.h"

#ifdef NQ_OS_WINDOWS

#include <windows.h>

#include <libnetq/string/String.h>
#include <libnetq/Assert.h>
  
#define WM_USER_DISPATCH (WM_USER + 1)

void NQPlatformLooper_init(NQPlatformLooper* thiz)
{
  NQMemset(thiz->eventHandle, 0, sizeof(thiz->eventHandle));
  thiz->eventTotal = 0;
  thiz->tid = NQThreadId();
}

void NQPlatformLooper_finilize(NQPlatformLooper* thiz)
{
}

void NQPlatformLooper_wake(NQPlatformLooper* thiz)
{
  BOOL result;

  if (thiz->tid == 0 || thiz->tid == NQThreadId())
    result = PostMessageW(NULL, WM_USER_DISPATCH, 0, 0);
  else
    result = PostThreadMessageW((DWORD)thiz->tid, WM_USER_DISPATCH, 0, 0);

  NQ_ASSERT_UNUSED(result, result);
}

void NQPlatformLooper_stop(NQPlatformLooper* thiz)
{
  PostQuitMessage(0);
}

int NQPlatformLooper_poll(NQPlatformLooper* thiz, int64_t timeout)
{
  DWORD dwMilliseconds = (timeout == -1) ? INFINITE : (DWORD)(timeout);
  DWORD dwWakeMask = QS_ALLINPUT | QS_ALLPOSTMESSAGE;
  DWORD dwFlags = MWMO_ALERTABLE | MWMO_INPUTAVAILABLE;

  DWORD waitResult = MsgWaitForMultipleObjectsEx(thiz->eventTotal, thiz->eventHandle, dwMilliseconds, dwWakeMask, dwFlags);

  if (waitResult == WAIT_FAILED)
    return NQ_LOOPER_POLL_ERROR;

  if (waitResult == WAIT_TIMEOUT)
    return NQ_LOOPER_POLL_TIMEOUT;

  if (waitResult == thiz->eventTotal) {
    return NQ_LOOPER_POLL_MESSAGE;
  }
  else if (WAIT_OBJECT_0 <= waitResult && waitResult <= WAIT_OBJECT_0 + thiz->eventTotal) {
    return NQ_LOOPER_POLL_SOURCE;
  }

  NQ_ASSERT_NOT_REACHED();
  return NQ_LOOPER_POLL_ERROR;
}

bool NQPlatformLooper_getMessage(NQPlatformLooper* thiz, NQMessage* message)
{
  MSG msg;

  if (!PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    return false;

  if (msg.message == WM_USER_DISPATCH)
    message->type = NQ_MESSAGE_WAKEUP;
  else {
    if (msg.message == WM_QUIT)
      message->type = NQ_MESSAGE_QUIT;
    else
      message->type = NQ_MESSAGE_NORMAL;

    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  return true;
}

#endif
