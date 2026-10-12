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

/* Touches of fingers (or a stylus) on a touch screen, as the window
 * system reports them, with no gestures made of them yet; a mouse comes
 * as NQMouseEvent instead.
 *
 *   NQ_EVENT_MOTION_START   a finger went down: the first of a gesture,
 *                           or another one while others are down
 *   NQ_EVENT_MOTION_MOVE    one or more of the fingers down moved
 *   NQ_EVENT_MOTION_END     a finger went up; the last one ends the gesture
 *   NQ_EVENT_MOTION_CANCEL  the system took the gesture over (a system
 *                           gesture, the window going away): all of the
 *                           fingers are gone, and nothing they did counts
 *
 * Each event lists every finger down, the one that went up included in
 * NQ_EVENT_MOTION_END, the first to go down first. The ones the event is
 * about are flagged NQ_TOUCHPOINT_CHANGED: the one that went down or up,
 * the ones that moved. */

/* the event is about this finger */
#define NQ_TOUCHPOINT_CHANGED (1 << 0)

typedef struct NQTouchPoint {
  uint32_t identifier;  /* the same for a finger from its start to its end */
  uint32_t flags;       /* NQ_TOUCHPOINT_* */
  int32_t positionX;    /* in the window's client area, in the units of */
  int32_t positionY;    /* NQMouseEvent.clientX / clientY */
} NQTouchPoint;

typedef struct NQMotionEvent {
  int type;             /* NQ_EVENT_MOTION_* */
  uint32_t count;
  NQTouchPoint* points; /* `count` of them; valid while the event is handled */
} NQMotionEvent;

#endif /* _NETQ_INPUT_MOTIONEVENT_H */
