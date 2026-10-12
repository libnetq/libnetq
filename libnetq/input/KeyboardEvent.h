/*
 * MIT License
 *
 * Copyright (c) 2021-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_INPUT_KEYBOARDEVENT_H
#define _LIBNETQ_INPUT_KEYBOARDEVENT_H

#include <libnetq/Event.h>

#define NQ_KBMOD_NONE     0x0000u
#define NQ_KBMOD_SHIFT    0x0001u
#define NQ_KBMOD_CTRL     0x0002u
#define NQ_KBMOD_ALT      0x0004u   /* Option on macOS */
#define NQ_KBMOD_SUPER    0x0008u   /* Win / Command / Meta */
#define NQ_KBMOD_CAPSLOCK 0x0010u
#define NQ_KBMOD_NUMLOCK  0x0020u
#define NQ_KBMOD_FN       0x0040u   /* macOS / Android Fn */

typedef struct NQKeyboardEvent {
  int type;
  uint32_t scancode;
  uint32_t repeatCount;
  uint32_t modifiers;
  char key;
} NQKeyboardEvent;

#endif /* _LIBNETQ_INPUT_KEYBOARDEVENT_H */
