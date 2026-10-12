/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_GFX_BLEND2D_CANVAS_H
#define _LIBNETQ_GFX_BLEND2D_CANVAS_H
#ifdef CANVAS_BACKEND_BLEND2D

#include <libnetq/Assert.h>
#include <libnetq/Malloc.h>
#include <libnetq/gfx/ColorUtils.h>
#include <libnetq/Math.h>

#include <blend2d/blend2d.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_CANVAS_BACKEND_NAME "blend2d"

typedef BLContextCore NQCanvasContext;
typedef BLPathCore NQCanvasPath;
typedef BLStrokeCap NQCanvasLineCap;
typedef BLStrokeJoin NQCanvasLineJoin;
typedef BLFontManagerCore NQCanvasFontFaceCache;
typedef BLFontFaceCore NQCanvasFontFace;
typedef BLFontCore NQCanvasFont;
typedef double NQCanvasFloat;
/* Both ascent and descent are positive */
typedef BLFontMetrics NQCanvasFontMetrics;
/* A PRGB32 image the context paints from */
typedef BLImageCore NQCanvasImage;

/* A Blend2D context draws whole paths, so the canvas keeps the current path
 * itself, and the font at its current size; the context paints into the
 * image, and all of them belong to the NQCanvas */
typedef struct NQCanvas NQCanvas;
struct NQCanvas {
  BLImageCore image;
  BLContextCore context;
  BLPathCore path;
  BLFontCore font;
  NQCanvasFloat fontSize;
  NQTextState text;
};

#define toCanvasFloat(value) (value)
#define getCanvasContext(thiz) (&(thiz)->context)
#define getCanvasPath(thiz) (&(thiz)->path)
/* the sized font made from the NQCanvasFontFace, not the face itself */
#define getCanvasFont(thiz) (&(thiz)->font)
#define getCanvasTextState(thiz) (&(thiz)->text)

static inline void NQCanvasFontFaceCache_init(NQCanvasFontFaceCache* thiz)
{
  bl_font_manager_init_new(thiz);
}

static inline void NQCanvasFontFaceCache_finalize(NQCanvasFontFaceCache* thiz)
{
  bl_font_manager_destroy(thiz);
}

NQ_EXPORT void NQCanvasFontFaceCache_loadSystem(NQCanvasFontFaceCache*);

static inline void NQCanvasContext_imageSize(NQCanvasContext* thiz, NQCanvasImage* image, int* w, int* h)
{
  BLImageData data;
  if (bl_image_get_data(image, &data) == BL_SUCCESS) {
    *w = data.size.w;
    *h = data.size.h;
  }
  else {
    *w = 0;
    *h = 0;
  }
}
#define NQCanvasContext_imageSize NQCanvasContext_imageSize

static inline void NQCanvasContext_save(NQCanvasContext* thiz)
{
  bl_context_save(thiz, NULL);
}
#define NQCanvasContext_save NQCanvasContext_save

static inline void NQCanvasContext_restore(NQCanvasContext* thiz)
{
  bl_context_restore(thiz, NULL);
}
#define NQCanvasContext_restore NQCanvasContext_restore

/* Blend2D clips to rectangles only, so NQCanvas_clip has no path clip here */
static inline void NQCanvasContext_clipRect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  const BLRect rect = { x, y, w, h };
  bl_context_clip_to_rect_d(thiz, &rect);
}
#define NQCanvasContext_clipRect NQCanvasContext_clipRect

static inline void NQCanvasContext_translate(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  const NQCanvasFloat xy[2] = { x, y };
  bl_context_apply_transform_op(thiz, BL_TRANSFORM_OP_TRANSLATE, xy);
}
#define NQCanvasContext_translate NQCanvasContext_translate

static inline void NQCanvasContext_scale(NQCanvasContext* thiz, NQCanvasFloat sx, NQCanvasFloat sy)
{
  const NQCanvasFloat xy[2] = { sx, sy };
  bl_context_apply_transform_op(thiz, BL_TRANSFORM_OP_SCALE, xy);
}
#define NQCanvasContext_scale NQCanvasContext_scale

static inline void NQCanvasContext_rotate(NQCanvasContext* thiz, NQCanvasFloat angle)
{
  bl_context_apply_transform_op(thiz, BL_TRANSFORM_OP_ROTATE, &angle);
}
#define NQCanvasContext_rotate NQCanvasContext_rotate

static inline void NQCanvasContext_setColor(NQCanvasContext* thiz, NQCanvasFloat r, NQCanvasFloat g, NQCanvasFloat b, NQCanvasFloat a)
{
  const BLRgba rgba = { (float)r, (float)g, (float)b, (float)a };
  bl_context_set_fill_style_rgba(thiz, &rgba);
  bl_context_set_stroke_style_rgba(thiz, &rgba);
}
#define NQCanvasContext_setColor NQCanvasContext_setColor

static inline void NQCanvasContext_setLineWidth(NQCanvasContext* thiz, NQCanvasFloat v)
{
  bl_context_set_stroke_width(thiz, v);
}
#define NQCanvasContext_setLineWidth NQCanvasContext_setLineWidth

static inline NQCanvasLineCap toCanvasLineCap(NQLineCap lineCap)
{
  switch (lineCap) {
  case NQ_LINECAP_BUTT:
    return BL_STROKE_CAP_BUTT;
  case NQ_LINECAP_ROUND:
    return BL_STROKE_CAP_ROUND;
  case NQ_LINECAP_SQUARE:
    return BL_STROKE_CAP_SQUARE;
  }
  NQ_ASSERT_NOT_REACHED();
  return BL_STROKE_CAP_BUTT;
}

static inline void NQCanvasContext_setLineCap(NQCanvasContext* thiz, NQCanvasLineCap lineCap)
{
  bl_context_set_stroke_caps(thiz, lineCap);
}
#define NQCanvasContext_setLineCap NQCanvasContext_setLineCap

static inline NQCanvasLineJoin toCanvasLineJoin(NQLineJoin lineJoin)
{
  switch (lineJoin) {
  case NQ_LINEJOIN_MITER:
    /* past the miter limit a canvas miter falls back to a bevel */
    return BL_STROKE_JOIN_MITER_BEVEL;
  case NQ_LINEJOIN_ROUND:
    return BL_STROKE_JOIN_ROUND;
  case NQ_LINEJOIN_BEVEL:
    return BL_STROKE_JOIN_BEVEL;
  }
  NQ_ASSERT_NOT_REACHED();
  return BL_STROKE_JOIN_MITER_BEVEL;
}

static inline void NQCanvasContext_setLineJoin(NQCanvasContext* thiz, NQCanvasLineJoin lineJoin)
{
  bl_context_set_stroke_join(thiz, lineJoin);
}
#define NQCanvasContext_setLineJoin NQCanvasContext_setLineJoin

static inline void NQCanvasContext_setMiterLimit(NQCanvasContext* thiz, NQCanvasFloat miterLimit)
{
  bl_context_set_stroke_miter_limit(thiz, miterLimit);
}
#define NQCanvasContext_setMiterLimit NQCanvasContext_setMiterLimit

static inline void NQCanvasPath_beginPath(NQCanvasPath* thiz)
{
  bl_path_clear(thiz);
}
#define NQCanvasPath_beginPath NQCanvasPath_beginPath

static inline void NQCanvasPath_moveTo(NQCanvasPath* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  bl_path_move_to(thiz, x, y);
}
#define NQCanvasPath_moveTo NQCanvasPath_moveTo

static inline void NQCanvasPath_lineTo(NQCanvasPath* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  bl_path_line_to(thiz, x, y);
}
#define NQCanvasPath_lineTo NQCanvasPath_lineTo

/* The sweep of an HTML canvas arc in the given direction: a whole turn at
 * most, otherwise the angle between start and end taken modulo one turn */
static inline double arcSweep(double startAngle, double endAngle, bool negative)
{
  const double sweep = negative ? startAngle - endAngle : endAngle - startAngle;
  double turns;
  if (sweep >= 2 * M_PI)
    turns = 2 * M_PI;
  else {
    turns = fmod(sweep, 2 * M_PI);
    if (turns < 0)
      turns += 2 * M_PI;
  }
  return negative ? -turns : turns;
}

/* Without force_move_to the arc is joined to the current point with a line */
static inline void NQCanvasPath_arc(NQCanvasPath* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat radius, NQCanvasFloat startAngle, NQCanvasFloat endAngle)
{
  bl_path_arc_to(thiz, x, y, radius, radius, startAngle, arcSweep(startAngle, endAngle, false), false);
}
#define NQCanvasPath_arc NQCanvasPath_arc

static inline void NQCanvasPath_arcNegative(NQCanvasPath* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat radius, NQCanvasFloat startAngle, NQCanvasFloat endAngle)
{
  bl_path_arc_to(thiz, x, y, radius, radius, startAngle, arcSweep(startAngle, endAngle, true), false);
}
#define NQCanvasPath_arcNegative NQCanvasPath_arcNegative

static inline bool NQCanvasPath_currentPoint(NQCanvasPath* thiz, NQCanvasFloat* x, NQCanvasFloat* y)
{
  BLPoint point;
  if (bl_path_get_size(thiz) == 0 || bl_path_get_last_vertex(thiz, &point) != BL_SUCCESS)
    return false;
  *x = point.x;
  *y = point.y;
  return true;
}
#define NQCanvasPath_currentPoint NQCanvasPath_currentPoint

static inline void NQCanvasPath_rect(NQCanvasPath* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  const BLRect rect = { x, y, w, h };
  bl_path_add_rect_d(thiz, &rect, BL_GEOMETRY_DIRECTION_CW);
}
#define NQCanvasPath_rect NQCanvasPath_rect

static inline void NQCanvasPath_roundedRect(NQCanvasPath* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h, NQCanvasFloat r)
{
  const BLRoundRect rect = { x, y, w, h, r, r };
  bl_path_add_geometry(thiz, BL_GEOMETRY_TYPE_ROUND_RECT, &rect, NULL, BL_GEOMETRY_DIRECTION_CW);
}
#define NQCanvasPath_roundedRect NQCanvasPath_roundedRect

static inline void NQCanvasPath_ellipse(NQCanvasPath* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  const BLEllipse ellipse = { x + w / 2, y + h / 2, w / 2, h / 2 };
  bl_path_add_geometry(thiz, BL_GEOMETRY_TYPE_ELLIPSE, &ellipse, NULL, BL_GEOMETRY_DIRECTION_CW);
}
#define NQCanvasPath_ellipse NQCanvasPath_ellipse

static inline void NQCanvasPath_closePath(NQCanvasPath* thiz)
{
  bl_path_close(thiz);
}
#define NQCanvasPath_closePath NQCanvasPath_closePath

static inline void NQCanvasContext_strokePath(NQCanvasContext* thiz, const NQCanvasPath* path)
{
  const BLPoint origin = { 0, 0 };
  bl_context_stroke_path_d(thiz, &origin, path);
}
#define NQCanvasContext_strokePath NQCanvasContext_strokePath

static inline void NQCanvasContext_fillPath(NQCanvasContext* thiz, const NQCanvasPath* path)
{
  const BLPoint origin = { 0, 0 };
  bl_context_fill_path_d(thiz, &origin, path);
}
#define NQCanvasContext_fillPath NQCanvasContext_fillPath

/* The shapes below are drawn as they are, leaving the current path alone */
static inline void NQCanvasContext_fillRect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  const BLRect rect = { x, y, w, h };
  bl_context_fill_rect_d(thiz, &rect);
}
#define NQCanvasContext_fillRect NQCanvasContext_fillRect

static inline void NQCanvasContext_strokeRect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  const BLRect rect = { x, y, w, h };
  bl_context_stroke_rect_d(thiz, &rect);
}
#define NQCanvasContext_strokeRect NQCanvasContext_strokeRect

static inline void NQCanvasContext_fillEllipse(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  const BLEllipse ellipse = { x + w / 2, y + h / 2, w / 2, h / 2 };
  bl_context_fill_geometry(thiz, BL_GEOMETRY_TYPE_ELLIPSE, &ellipse);
}
#define NQCanvasContext_fillEllipse NQCanvasContext_fillEllipse

static inline void NQCanvasContext_strokeEllipse(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  const BLEllipse ellipse = { x + w / 2, y + h / 2, w / 2, h / 2 };
  bl_context_stroke_geometry(thiz, BL_GEOMETRY_TYPE_ELLIPSE, &ellipse);
}
#define NQCanvasContext_strokeEllipse NQCanvasContext_strokeEllipse

static inline void NQCanvasFontFace_release(NQCanvasFontFace* thiz)
{
  bl_font_face_destroy(thiz);
  NQFree(thiz);
}
#define NQCanvasFontFace_release NQCanvasFontFace_release

static inline void NQCanvas_setFontFaceImpl(NQCanvas* thiz, NQCanvasFontFace* fontFace)
{
  bl_font_create_from_face(&thiz->font, fontFace, (float)thiz->fontSize);
}
#define NQCanvas_setFontFaceImpl NQCanvas_setFontFaceImpl

/* A BLFont has a fixed size, so the canvas keeps the size for NQCanvas_setFontFace */
static inline void NQCanvas_setFontSizeImpl(NQCanvas* thiz, NQCanvasFloat fontSize)
{
  thiz->fontSize = fontSize;
  bl_font_set_size(&thiz->font, (float)fontSize);
}
#define NQCanvas_setFontSizeImpl NQCanvas_setFontSizeImpl

static inline void NQCanvasFont_metrics(const NQCanvasFont* thiz, NQCanvasFontMetrics* fontMetrics)
{
  if (bl_font_get_metrics(thiz, fontMetrics) != BL_SUCCESS) {
    fontMetrics->ascent = 0;
    fontMetrics->descent = 0;
  }
}
#define NQCanvasFont_metrics NQCanvasFont_metrics

static inline NQCanvasFloat NQCanvasFontMetrics_ascent(const NQCanvasFontMetrics* thiz)
{
  return thiz->ascent;
}

static inline NQCanvasFloat NQCanvasFontMetrics_descent(const NQCanvasFontMetrics* thiz)
{
  return thiz->descent;
}

static inline double NQCanvas_textWidthImpl(NQCanvas* thiz, const char* text, size_t length)
{
  BLGlyphBufferCore glyphs;
  BLTextMetrics metrics;
  double width = 0;
  bl_glyph_buffer_init(&glyphs);
  if (bl_glyph_buffer_set_text(&glyphs, text, length, BL_TEXT_ENCODING_UTF8) == BL_SUCCESS &&
      bl_font_shape(&thiz->font, &glyphs) == BL_SUCCESS &&
      bl_font_get_text_metrics(&thiz->font, &glyphs, &metrics) == BL_SUCCESS)
    width = metrics.advance.x;
  bl_glyph_buffer_destroy(&glyphs);
  return width;
}
#define NQCanvas_textWidthImpl NQCanvas_textWidthImpl

/* Draw `length` bytes of text whose alphabetic baseline starts at (x, y) */
static inline void NQCanvasFont_strokeText(NQCanvasContext* thiz, const NQCanvasFont* font, const char* text, size_t length, NQCanvasFloat x, NQCanvasFloat y)
{
  const BLPoint origin = { x, y };
  bl_context_stroke_utf8_text_d(thiz, &origin, font, text, length);
}
#define NQCanvasFont_strokeText NQCanvasFont_strokeText

static inline void NQCanvasFont_fillText(NQCanvasContext* thiz, const NQCanvasFont* font, const char* text, size_t length, NQCanvasFloat x, NQCanvasFloat y)
{
  const BLPoint origin = { x, y };
  bl_context_fill_utf8_text_d(thiz, &origin, font, text, length);
}
#define NQCanvasFont_fillText NQCanvasFont_fillText

/* Lands every pending write in the pixels before someone else reads them */
static inline void NQCanvasImpl_flush(NQCanvas* thiz)
{
  bl_context_flush(&thiz->context, BL_CONTEXT_FLUSH_SYNC);
}
#define NQCanvasImpl_flush NQCanvasImpl_flush

static inline NQCanvasImage* NQCanvasImage_createRGBA(const void* pixels, int width, int height, int stride)
{
  NQCanvasImage* thiz = (NQCanvasImage*)NQMalloc(sizeof(NQCanvasImage));
  if (!thiz)
    return NULL;

  BLImageData data;
  if (bl_image_init_as(thiz, width, height, BL_FORMAT_PRGB32) != BL_SUCCESS || bl_image_make_mutable(thiz, &data) != BL_SUCCESS) {
    bl_image_destroy(thiz);
    NQFree(thiz);
    return NULL;
  }

  /* PRGB32 wants native-endian premultiplied 0xAARRGGBB words */
  const unsigned char* src = (const unsigned char*)pixels;
  for (int y = 0; y < height; y++) {
    const unsigned char* in = src + (size_t)stride * (size_t)y;
    uint32_t* out = (uint32_t*)((unsigned char*)data.pixel_data + (intptr_t)y * data.stride);
    NQRgbaToPremultipliedArgbLine((uint32_t)width, in, out);
  }

  return thiz;
}
#define NQCanvasImage_createRGBA NQCanvasImage_createRGBA

static inline void NQCanvasImage_release(NQCanvasImage* thiz)
{
  bl_image_destroy(thiz);
  NQFree(thiz);
}
#define NQCanvasImage_release NQCanvasImage_release

static inline void NQCanvasContext_drawImage(NQCanvasContext* thiz, NQCanvasImage* image,
    NQCanvasFloat sx, NQCanvasFloat sy, NQCanvasFloat sw, NQCanvasFloat sh,
    NQCanvasFloat dx, NQCanvasFloat dy, NQCanvasFloat dw, NQCanvasFloat dh)
{
  /* maps image pixels to user space so the source rect lands on the destination */
  const NQCanvasFloat ax = dw / sw;
  const NQCanvasFloat ay = dh / sh;

  BLMatrix2D matrix;
  matrix.m[0] = ax;
  matrix.m[1] = 0;
  matrix.m[2] = 0;
  matrix.m[3] = ay;
  matrix.m[4] = dx - sx * ax;
  matrix.m[5] = dy - sy * ay;

  /* filtering past the image's edge repeats the edge pixels, as the HTML
   * canvas does; a fill_rect leaves the canvas's own path alone */
  BLPatternCore pattern;
  bl_pattern_init_as(&pattern, image, NULL, BL_EXTEND_MODE_PAD, &matrix);
  const BLRect rect = { dx, dy, dw, dh };
  bl_context_fill_rect_d_ext(thiz, &rect, &pattern);
  bl_pattern_destroy(&pattern);
}
#define NQCanvasContext_drawImage NQCanvasContext_drawImage

#ifdef __cplusplus
}
#endif

#endif /* CANVAS_BACKEND_BLEND2D */
#endif /* _LIBNETQ_GFX_BLEND2D_CANVAS_H */
