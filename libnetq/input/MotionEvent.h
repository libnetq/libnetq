/*
 * MIT License
 *
 * Copyright (c) 2021-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _NETQ_INPUT_MOTIONEVENT_H
#define _NETQ_INPUT_MOTIONEVENT_H

#include <libnetq/Event.h>

#define NQ_TOUCHPOINT_CHANGED (1 << 0)

typedef struct NQTouchPoint {
  uint32_t identifier;
  uint32_t flags;
  int32_t positionX;
  int32_t positionY;
} NQTouchPoint;

typedef struct NQMotionEvent {
  int type;
  uint32_t count;
  NQTouchPoint* points;
} NQMotionEvent;

#endif /* _NETQ_INPUT_MOTIONEVENT_H */
