/*
 * MIT License
 *
 * Copyright (c) 2021-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/Signal.h"

#if !defined(NQ_OS_KERNEL) && !defined(NQ_OS_UNKNOWN)

#include <libnetq/sync/Once.h>
#include <libnetq/Mutex.h>
#include <libnetq/Malloc.h>
#include <libnetq/string/StringUtil.h>

#ifdef NQ_OS_WINDOWS
#include <windows.h>
typedef DWORD NQNativeSignalType;
#define NQ_SIGNAL_MAX (CTRL_SHUTDOWN_EVENT + 1)
typedef void* NQSignalNativeHandler;
#else
#include <signal.h>
typedef int NQNativeSignalType;

#ifdef NQ_OS_LINUX
#define NQ_SIGNAL_MAX _NSIG
typedef sighandler_t NQSignalNativeHandler;
#else
#define NQ_SIGNAL_MAX NSIG
typedef void (*NQSignalNativeHandler) (int);
#endif

#endif

typedef struct NQSignalEntry NQSignalEntry;
struct NQSignalEntry {
  NQSignalEntry* next;
  int type;
  NQNativeSignalType nativeType;
  void* userdata;
  NQSignalHandler handler;
};

typedef struct NQSignalInfo {
  NQSignalEntry* first;
#ifndef NQ_OS_WINDOWS
  NQSignalNativeHandler nativeHandler;
#endif
} NQSignalInfo;

static NQOnce gOnce = NQ_ONCE_INIT;
static NQMutex gMutex;
static NQSignalInfo gSignalMap[NQ_SIGNAL_MAX];

static bool convertToNativeType(int type, NQNativeSignalType* nativeType)
{
  NQNativeSignalType result;

  switch (type) {
#ifdef NQ_OS_WINDOWS
  case kNQSignalTypeCtrlC:
    result = CTRL_C_EVENT;
    break;

  case kNQSignalTypeCtrlBreak:
    result = CTRL_BREAK_EVENT;
    break;

  case kNQSignalTypeCtrlClose:
    result = CTRL_CLOSE_EVENT;
    break;

  case kNQSignalTypeCtrlLogoff:
    result = CTRL_LOGOFF_EVENT;
    break;

  case kNQSignalTypeCtrlShutdown:
    result = CTRL_SHUTDOWN_EVENT;
    break;
#else
  case kNQSignalTypeSigInt:
    result = SIGINT;
    break;

  case kNQSignalTypeSigTerm:
    result = SIGTERM;
    break;

  case kNQSignalTypeSigKill:
    result = SIGKILL;
    break;

  case kNQSignalTypeSigSegv:
    result = SIGSEGV;
    break;
#endif
  default:
    return false;
  };

  if (nativeType != NULL)
    *nativeType = result;

  return true;
}

static void executeSignalList(NQSignalEntry* entry, NQNativeSignalType nativeType)
{
  while (entry != NULL) {
    if (entry->nativeType == nativeType)
      entry->handler(entry->type, entry->userdata);
    entry = entry->next;
  }
}

#ifdef NQ_OS_WINDOWS
static BOOL WINAPI onSignalHandler(NQNativeSignalType nativeType)
{
  if (nativeType < NQ_SIGNAL_MAX) {
    executeSignalList(gSignalMap[nativeType].first, nativeType);
  }

  return FALSE;
}

#else
static void onSignalHandler(NQNativeSignalType nativeType)
{
  if (nativeType < NQ_SIGNAL_MAX && gSignalMap[nativeType].nativeHandler) {
    executeSignalList(gSignalMap[nativeType].first, nativeType);
    gSignalMap[nativeType].nativeHandler(nativeType);
  }
}

#endif

static void NQSignalInit(void)
{
  NQMutex_init(&gMutex);

#ifdef NQ_OS_WINDOWS
  SetConsoleCtrlHandler(onSignalHandler, TRUE);
  
#else
  int n;
  struct sigaction targetSigAction;
  struct sigaction otherSigAction;

  memset(&targetSigAction, 0, sizeof(targetSigAction));
  targetSigAction.sa_handler = onSignalHandler;
  sigemptyset(&targetSigAction.sa_mask);

  for (n = 0; n < (int)NQ_ARRAY_LENGTH(gSignalMap); n++) {
    gSignalMap[n].first = NULL;
    if (convertToNativeType(n, NULL)) {
      sigaction(n, &targetSigAction, &otherSigAction);
      gSignalMap[n].nativeHandler = otherSigAction.sa_handler;
    }
    else {
      gSignalMap[n].nativeHandler = NULL;
    }
  }

#endif
}

bool NQSetSignalHandler(int type, NQSignalHandler handler, void* userdata)
{
  NQOnce_call(&gOnce, NQSignalInit);

  NQNativeSignalType nativeType;

  if (!convertToNativeType(type, &nativeType))
    return false;

  if (NQ_ARRAY_LENGTH(gSignalMap) <= (size_t)nativeType)
    return false;

  NQSignalEntry* entry = (NQSignalEntry*)NQMalloc(sizeof(NQSignalEntry));
  if (entry == NULL)
    return false;

  entry->type = type;
  entry->nativeType = nativeType;
  entry->userdata = userdata;
  entry->handler = handler;

  NQSignalInfo* info = &gSignalMap[nativeType];

  NQMutex_lock(&gMutex);
  entry->next = info->first;
  info->first = entry;
  NQMutex_unlock(&gMutex);

  return true;
}

#else

bool NQSetSignalHandler(int type, NQSignalHandler handler, void* userdata)
{
  NQ_UNUSED_PARAM(type);
  NQ_UNUSED_PARAM(handler);
  NQ_UNUSED_PARAM(userdata);

  return false;
}

#endif
