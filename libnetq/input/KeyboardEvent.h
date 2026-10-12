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

typedef enum NQKey {
  NQ_KEY_NONE = 0,
  NQ_KEY_CHAR,        /* pressed with Ctrl / Command, or Alt where that types nothing, see codepoint */
  NQ_KEY_BACKSPACE,
  NQ_KEY_DELETE,
  NQ_KEY_LEFT,
  NQ_KEY_RIGHT,
  NQ_KEY_UP,
  NQ_KEY_DOWN,
  NQ_KEY_HOME,
  NQ_KEY_END,
  NQ_KEY_PAGE_UP,
  NQ_KEY_PAGE_DOWN,
  NQ_KEY_RETURN,
  NQ_KEY_TAB,
  NQ_KEY_ESCAPE,
  NQ_KEY_INSERT,
  NQ_KEY_F1,          /* to NQ_KEY_F12, in order */
  NQ_KEY_F2,
  NQ_KEY_F3,
  NQ_KEY_F4,
  NQ_KEY_F5,
  NQ_KEY_F6,
  NQ_KEY_F7,
  NQ_KEY_F8,
  NQ_KEY_F9,
  NQ_KEY_F10,
  NQ_KEY_F11,
  NQ_KEY_F12,
} NQKey;

typedef struct NQKeyboardEvent {
  int type;             /* NQ_EVENT_KEY_DOWN, NQ_EVENT_KEY_UP or NQ_EVENT_CHAR */
  uint32_t scancode;    /* the platform's own code */
  uint32_t repeatCount;
  uint32_t modifiers;   /* NQ_KBMOD_* */
  char key;
  NQKey keyCode;        /* which key, on any platform */
  uint32_t codepoint;   /* NQ_KEY_CHAR: the unshifted character, letters lower case; with Alt
                           alone the character the key gives, as far as the platform can tell */
  const char* text;     /* NQ_EVENT_CHAR: NUL-terminated UTF-8, valid during the call */
} NQKeyboardEvent;

#endif /* _LIBNETQ_INPUT_KEYBOARDEVENT_H */
