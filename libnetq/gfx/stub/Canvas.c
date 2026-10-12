/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/gfx/Canvas.h"
#include "libnetq/gfx/stub/Canvas.h"

#ifdef CANVAS_BACKEND_STUB

void NQCanvas_release(NQCanvas* thiz)
{
  NQ_UNUSED_PARAM(thiz);
}

void NQGFont_release(NQGFont* thiz)
{
  NQ_UNUSED_PARAM(thiz);
}

void NQCanvas_setFontSize(NQCanvas* thiz, double fontSize)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(fontSize);
}

NQGFontMetrics NQCanvas_fontMetrics(NQCanvas* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  NQGFontMetrics result = { .ascent = 0, .descent = 0 };
  return result;
}

double NQCanvas_textWidth(NQCanvas* thiz, const char* text)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(text);
  return 0.0;
}

double NQCanvas_textWidth2(NQCanvas* thiz, const char* text, size_t length)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(text);
  NQ_UNUSED_PARAM(length);
  return 0.0;
}

#endif /* CANVAS_BACKEND_STUB */
