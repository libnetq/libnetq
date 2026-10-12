/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_GFX_COLOR_H
#define _LIBNETQ_GFX_COLOR_H

#include <libnetq/Basic.h>

typedef uint32_t NQColorRgba;

typedef struct NQColorRgba8 NQColorRgba8;
struct NQColorRgba8 {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};

typedef struct NQColorRgba16 NQColorRgba16;
struct NQColorRgba16 {
  uint16_t r;
  uint16_t g;
  uint16_t b;
  uint16_t a;
};

typedef struct NQColorRgba32 NQColorRgba32;
struct NQColorRgba32 {
  uint32_t r;
  uint32_t g;
  uint32_t b;
  uint32_t a;
};

typedef struct NQColorRgbaF NQColorRgbaF;
struct NQColorRgbaF {
  float r;
  float g;
  float b;
  float a;
};

typedef struct NQColorRgbaD NQColorRgbaD;
struct NQColorRgbaD {
  double r;
  double g;
  double b;
  double a;
};

#define NQ_COLORRGBA_INIT(r, g, b, a) { r, g, b, a }

#endif /* _LIBNETQ_GFX_COLOR_H */
