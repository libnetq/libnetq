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

/* Takes over the surface, destroying it if the context cannot be made */
static NQCanvas* NQCanvas_fromSurface(cairo_surface_t* surface)
{
  if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
    cairo_surface_destroy(surface);
    return NULL;
  }
  NQCanvas* thiz = (NQCanvas*)NQMalloc(sizeof(NQCanvas));
  if (!thiz) {
    cairo_surface_destroy(surface);
    return NULL;
  }
  thiz->surface = surface;
  thiz->canvas = cairo_create(surface);
  thiz->text = (NQTextState){0};
  if (cairo_status(thiz->canvas) != CAIRO_STATUS_SUCCESS) {
    cairo_destroy(thiz->canvas);
    cairo_surface_destroy(surface);
    NQFree(thiz);
    return NULL;
  }
  return thiz;
}

NQCanvas* NQCanvas_create(int width, int height)
{
  if (width <= 0 || height <= 0)
    return NULL;
  return NQCanvas_fromSurface(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height));
}

/* The pixels are native-endian premultiplied 0xAARRGGBB words; they stay
 * the caller's and must outlive the canvas */
NQCanvas* NQCanvas_createForData(void* pixels, int width, int height, int stride)
{
  if (!pixels || width <= 0 || height <= 0)
    return NULL;
  return NQCanvas_fromSurface(
      cairo_image_surface_create_for_data((unsigned char*)pixels, CAIRO_FORMAT_ARGB32, width, height, stride));
}

void NQCanvas_release(NQCanvas* thiz)
{
  if (!thiz)
    return;
  NQCanvasContext_release(thiz->canvas); /* before the surface it paints into */
  cairo_surface_finish(thiz->surface);
  cairo_surface_destroy(thiz->surface);
  NQFree(thiz);
}

/* Glyphs placed from (x, y) need no current point, so the path is left alone */
static cairo_glyph_t* NQCanvasContext_textGlyphs(NQCanvasContext* thiz, const char* text, size_t length, double x, double y, int* numGlyphs)
{
  cairo_glyph_t* glyphs = NULL;
  NQCanvasFont* scaledFont = cairo_get_scaled_font(thiz);
  if (cairo_scaled_font_text_to_glyphs(scaledFont, x, y, text, (int)length, &glyphs, numGlyphs, NULL, NULL, NULL) != CAIRO_STATUS_SUCCESS)
    return NULL;
  return glyphs;
}

/* The outlines go through the path, which is put back afterwards */
static void NQCanvasContext_drawGlyphOutlines(NQCanvasContext* thiz, cairo_glyph_t* glyphs, int numGlyphs, void (*draw)(cairo_t*))
{
  cairo_path_t* path = cairo_copy_path(thiz);
  cairo_new_path(thiz);
  cairo_glyph_path(thiz, glyphs, numGlyphs);
  draw(thiz);
  cairo_append_path(thiz, path);
  cairo_path_destroy(path);
}

void NQCanvasContext_strokeText(NQCanvasContext* thiz, const char* text, size_t length, double x, double y)
{
  int numGlyphs = 0;
  cairo_glyph_t* glyphs = NQCanvasContext_textGlyphs(thiz, text, length, x, y, &numGlyphs);
  if (!glyphs)
    return;
  NQCanvasContext_drawGlyphOutlines(thiz, glyphs, numGlyphs, cairo_stroke);
  cairo_glyph_free(glyphs);
}

void NQCanvasContext_fillText(NQCanvasContext* thiz, const char* text, size_t length, double x, double y)
{
  int numGlyphs = 0;
  cairo_glyph_t* glyphs = NQCanvasContext_textGlyphs(thiz, text, length, x, y, &numGlyphs);
  if (!glyphs)
    return;
#ifdef _WIN32
  /* cairo's GDI font backend clips hinted glyph bitmaps to the GDI black
   * box (losing e.g. the dot of "i") and renders ClearType as a colour
   * mask, so fill the glyph outlines with cairo's own rasterizer instead. */
  NQCanvasContext_drawGlyphOutlines(thiz, glyphs, numGlyphs, cairo_fill);
#else
  cairo_show_glyphs(thiz, glyphs, numGlyphs);
#endif
  cairo_glyph_free(glyphs);
}

NQCanvasFontFace* NQCanvasFontFace_createFromFamily(NQCanvas* cr, const char* family, NQFontSlant slant, NQFontWeight weight)
{
  NQCanvasFontFace* fontFace = cairo_toy_font_face_create(family, toCanvasFontSlant(slant), toCanvasFontWeight(weight));
  if (cairo_font_face_status(fontFace) != CAIRO_STATUS_SUCCESS) {
    cairo_font_face_destroy(fontFace);
    return NULL;
  }
  return fontFace;
}

#endif /* CANVAS_BACKEND_CAIRO */
