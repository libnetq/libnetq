/*
 * MIT License
 *
 * Copyright (c) 2021-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_INPUT_MOUSEEVENT_H
#define _LIBNETQ_INPUT_MOUSEEVENT_H

#include <libnetq/Event.h>

typedef uint8_t NQMouseButton;
enum {
  kNQMouseButtonNone,
  kNQMouseButtonLeft,
  kNQMouseButtonMiddle,
  kNQMouseButtonRight,
};

typedef struct NQMouseEvent {
  int type;
  int32_t clientX;
  int32_t clientY;
  int32_t screenX;
  int32_t screenY;
  uint16_t state;
  NQMouseButton button;
  uint8_t reserve;
} NQMouseEvent;

#endif /* _LIBNETQ_INPUT_MOUSEEVENT_H */
