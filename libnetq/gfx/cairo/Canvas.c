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
#include "libnetq/gfx/cairo/Canvas.h"

#ifdef CANVAS_BACKEND_CAIRO

#include <libnetq/Malloc.h>
#include <libnetq/Math.h>

NQCanvas* NQCanvas_fromCairo(cairo_t* cr)
{
  if (!cr)
    return NULL;
  NQCanvas* thiz = (NQCanvas*)NQMalloc(sizeof(NQCanvas));
  if (!thiz)
    return NULL;
  thiz->canvas = cairo_reference(cr);
  return thiz;
}

/* Drops the canvas's reference; the context is destroyed with the last one. */
void NQCanvas_release(NQCanvas* thiz)
{
  NQCanvasContext_release(thiz->canvas);
  NQFree(thiz);
}

void NQGFont_release(NQGFont* thiz)
{
  cairo_font_face_destroy(thiz);
}

void NQCanvas_setFontSize(NQCanvas* thiz, double fontSize)
{
  cairo_set_font_size(thiz->canvas, fontSize);
}

NQGFontMetrics NQCanvas_fontMetrics(NQCanvas* thiz)
{
  cairo_font_extents_t fext;
  cairo_font_extents(thiz->canvas, &fext);

  NQGFontMetrics result;
  result.ascent = fext.ascent;
  result.descent = fext.descent;

  return result;
}

double NQCanvas_textWidth(NQCanvas* thiz, const char* text)
{
  cairo_text_extents_t ext;
  cairo_text_extents(thiz->canvas, text, &ext);
  return ext.x_advance;
}

double NQCanvas_textWidth2(NQCanvas* thiz, const char *text, size_t length)
{
  if (length == 0)
    return 0;

  cairo_status_t status;
  cairo_glyph_t* glyphs = NULL;
  int numGlyphs = 0;
  cairo_scaled_font_t* scaledFont = cairo_get_scaled_font(thiz->canvas);

  status = cairo_scaled_font_text_to_glyphs(scaledFont, 0, 0, text, (int)length, &glyphs, &numGlyphs, NULL, NULL, NULL);
  if (status != CAIRO_STATUS_SUCCESS)
    return 0;

  cairo_text_extents_t ext;
  cairo_glyph_extents(thiz->canvas, glyphs, numGlyphs, &ext);
  cairo_glyph_free(glyphs);
  return ext.x_advance;
}

#endif /* CANVAS_BACKEND_CAIRO */
