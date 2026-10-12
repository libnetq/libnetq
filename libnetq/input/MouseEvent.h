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
  NQ_MOUSE_BUTTON_NONE = 0,
  NQ_MOUSE_BUTTON_LEFT,
  NQ_MOUSE_BUTTON_MIDDLE,
  NQ_MOUSE_BUTTON_RIGHT,
};

/* What wheelX and wheelY count for one notch of a wheel, as on Windows;
 * trackpads and fine wheels report parts of it */
#define NQ_WHEEL_NOTCH 120

typedef struct NQMouseEvent {
  int type;
  int32_t clientX;
  int32_t clientY;
  int32_t screenX;      /* 0 where the platform has none (Wayland) */
  int32_t screenY;
  uint16_t state;       /* buttons held: 1 left, 2 right, 4 middle */
  NQMouseButton button;
  uint8_t clickCount;   /* NQ_EVENT_MOUSE_PRESSED: 1 single, 2 double, ... as the platform's
                           double-click time and distance count them; 0 where not counted */
  uint32_t modifiers;   /* NQ_KBMOD_* */
  int32_t wheelX;       /* NQ_EVENT_MOUSE_SCROLL: positive to the right */
  int32_t wheelY;       /* NQ_EVENT_MOUSE_SCROLL: positive away from the user */
} NQMouseEvent;

#endif /* _LIBNETQ_INPUT_MOUSEEVENT_H */
