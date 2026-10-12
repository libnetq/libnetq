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
#include "libnetq/gfx/nanovg/Canvas.h"

#ifdef CANVAS_BACKEND_NANOVG

#include <libnetq/string/String.h>

void NQCanvas_release(NQCanvas* thiz)
{
  NQCanvasContext_release(toCanvasContext(thiz));
}

void NQCanvas_setFontSize(NQCanvas* thiz, double fontSize)
{
  nvgFontSize(thiz, (float)fontSize);
}

NQGFontMetrics NQCanvas_fontMetrics(NQCanvas* thiz)
{
  float ascent = 0, descent = 0;
  nvgTextMetrics(thiz, &ascent, &descent, NULL);

  /* NanoVG reports the descent below the baseline as a negative number */
  NQGFontMetrics result;
  result.ascent = ascent;
  result.descent = (0 - descent);

  return result;
}

double NQCanvas_textWidth(NQCanvas* thiz, const char* text)
{
  return nvgTextBounds(thiz, 0, 0, text, text + NQStrlen(text), NULL);
}

double NQCanvas_textWidth2(NQCanvas* thiz, const char* text, size_t length)
{
  return nvgTextBounds(thiz, 0, 0, text, text + length, NULL);
}

#endif /* CANVAS_BACKEND_NANOVG */
