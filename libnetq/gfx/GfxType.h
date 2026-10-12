/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_GFX_GFXTYPES_H
#define _LIBNETQ_GFX_GFXTYPES_H

#include <libnetq/Basic.h>

typedef enum NQLineCap {
  NQ_LINECAP_BUTT,
  NQ_LINECAP_ROUND,
  NQ_LINECAP_SQUARE,
} NQLineCap;

NQ_EXPORT const char* NQLineCapToString(NQLineCap lineCap);

typedef enum NQLineJoin {
  NQ_LINEJOIN_MITER,
  NQ_LINEJOIN_ROUND,
  NQ_LINEJOIN_BEVEL,
} NQLineJoin;

NQ_EXPORT const char* NQLineJoinToString(NQLineJoin lineJoin);

typedef enum NQFontSlant {
  NQ_FONT_SLANT_NORMAL,
  NQ_FONT_SLANT_ITALIC,
  NQ_FONT_SLANT_OBLIQUE,
} NQFontSlant;

typedef enum NQFontWeight {
  NQ_FONT_WEIGHT_NORMAL,
  NQ_FONT_WEIGHT_BOLD,
} NQFontWeight;

typedef enum {
  NQ_TEXT_ALIGN_START,
  NQ_TEXT_ALIGN_END,
  NQ_TEXT_ALIGN_LEFT,
  NQ_TEXT_ALIGN_RIGHT,
  NQ_TEXT_ALIGN_CENTER
} NQTextAlign;

typedef enum {
  NQ_TEXT_BASELINE_ALPHABETIC,
  NQ_TEXT_BASELINE_TOP,
  NQ_TEXT_BASELINE_HANGING,
  NQ_TEXT_BASELINE_MIDDLE,
  NQ_TEXT_BASELINE_IDEOGRAPHIC,
  NQ_TEXT_BASELINE_BOTTOM
} NQTextBaseline;

typedef enum {
  NQ_TEXT_DIRECTION_LTR,
  NQ_TEXT_DIRECTION_RTL
} NQTextDirection;

#endif /* _LIBNETQ_GFX_GFXTYPES_H */
