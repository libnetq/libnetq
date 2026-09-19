/*
 * MIT License
 *
 * Copyright (c) 2021-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_LOOPERSOURCE_H
#define _LIBNETQ_LOOPERSOURCE_H

#include <libnetq/net/SocketHandle.h>

#ifdef NQ_OS_ANDROID
#include <android/input.h>
#endif

#ifdef NQ_OS_WINDOWS
#include <windows.h>
#endif

#include <libnetq/Event.h>
#include <libnetq/fs/FileHandle.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQLooper NQLooper;
typedef struct NQLooperSource NQLooperSource;

typedef void (*NQSourceHandleCallback) (void* userdata, const NQEvent* e);

enum {
  NQ_SOURCE_FD,
  NQ_SOURCE_FILE,
  NQ_SOURCE_SOCKET,
  NQ_SOURCE_AINPUT,
  NQ_SOURCE_HANDLE,
};

struct NQLooperSource {
  int type;

  void* userdata;
  NQSourceHandleCallback handleEvent;

  union {
    int (*getFileDescriptor) (void* userdata);
    NQFileHandle (*getFileHandle) (void* userdata);
    NQSocketHandle (*getSocketHandle) (void* userdata);

#ifdef NQ_OS_ANDROID
    AInputQueue* (*getInputQueue) (void* userdata);
#endif

#ifdef NQ_OS_WINDOWS
    HANDLE (*getNativeHandle) (void* userdata);
#endif
  } u;
};


#ifdef NQ_OS_ANDROID
struct NQInputEvent {
  int type;
  AInputQueue* inputQueue;
  AInputEvent* inputEvent;
};

NQ_EXPORT bool NQLooperSource_getInputQueue(const NQLooperSource* src, AInputQueue** result);
#endif


#ifdef NQ_OS_WINDOWS
struct NQHandleEvent {
  int type;
  HANDLE handle;
};

NQ_EXPORT bool NQLooperSource_getNativeHandle(const NQLooperSource* src, HANDLE* result);
#endif


struct NQFDEvent {
  int type;
  int fd;
};

NQ_EXPORT bool NQLooperSource_getFileDescriptor(const NQLooperSource* src, int* result);


struct NQSocketEvent {
  int type;
  NQSocketHandle socket;
};

NQ_EXPORT bool NQLooperSource_getSocketHandle(const NQLooperSource* src, NQSocketHandle* result);


NQ_EXPORT void NQLooperSource_destroy(NQLooperSource* src);

NQ_EXPORT void NQLooperSource_handleEvent(NQLooperSource* src, const NQEvent* e);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_LOOPERSOURCE_H */
