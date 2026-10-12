/*
 * MIT License
 *
 * Copyright (c) 2021-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_INPUT_WINDOWEVENT_H
#define _LIBNETQ_INPUT_WINDOWEVENT_H

#include <libnetq/Event.h>
#include <libnetq/gfx/Rect.h>

typedef struct NQWindowCreateEvent {
  int type;
} NQWindowCreateEvent;

typedef struct NQWindowDestroyEvent {
  int type;
} NQWindowDestroyEvent;

typedef struct NQWindowResizedEvent {
  int type;
  NQRect rect;
} NQWindowResizedEvent;

typedef struct NQWindowShowEvent {
  int type;
} NQWindowShowEvent;

typedef struct NQWindowHideEvent {
  int type;
} NQWindowHideEvent;

typedef struct NQWindowDrawEvent {
  int type;
  void* gc;
} NQWindowDrawEvent;

#endif /* _LIBNETQ_INPUT_WINDOWEVENT_H */
