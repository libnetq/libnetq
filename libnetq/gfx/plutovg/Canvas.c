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
#include "libnetq/gfx/plutovg/Canvas.h"

#ifdef CANVAS_BACKEND_PLUTOVG

void NQCanvas_release(NQCanvas* thiz)
{
  NQCanvasContext_release(toCanvasContext(thiz));
}

void NQGFont_release(NQGFont* thiz)
{
  plutovg_font_face_destroy(thiz);
}

void NQCanvas_setFontSize(NQCanvas* thiz, double fontSize)
{
  plutovg_canvas_set_font_size(thiz, (float)fontSize);
}

NQGFontMetrics NQCanvas_fontMetrics(NQCanvas* thiz)
{
  float ascent = 0, descent = 0;
  plutovg_canvas_font_metrics(thiz, &ascent, &descent, NULL, NULL);

  /* PlutoVG reports the descent below the baseline as a negative number */
  NQGFontMetrics result;
  result.ascent = ascent;
  result.descent = (0 - descent);

  return result;
}

double NQCanvas_textWidth(NQCanvas* thiz, const char* text)
{
  return plutovg_canvas_text_extents(thiz, text, -1, PLUTOVG_TEXT_ENCODING_UTF8, NULL);
}

double NQCanvas_textWidth2(NQCanvas* thiz, const char* text, size_t length)
{
  return plutovg_canvas_text_extents(thiz, text, (int)length, PLUTOVG_TEXT_ENCODING_UTF8, NULL);
}

#endif /* CANVAS_BACKEND_PLUTOVG */
