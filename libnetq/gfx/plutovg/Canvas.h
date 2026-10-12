/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_GFX_PLUTOVG_CANVAS_H
#define _LIBNETQ_GFX_PLUTOVG_CANVAS_H
#ifdef CANVAS_BACKEND_PLUTOVG

#include <libnetq/Assert.h>
#include <libnetq/gfx/GfxType.h>

#include <plutovg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_CANVAS_DEFINED_INTERNAL
#define NQ_CANVAS_BACKEND_NAME "plutovg"

typedef struct NQCanvas NQCanvas;
/* The canvas paints into the surface; both belong to the NQCanvas */
struct NQCanvas {
  plutovg_surface_t* surface;
  plutovg_canvas_t* canvas;
  NQTextState text;
};

typedef plutovg_canvas_t NQCanvasContext;
typedef plutovg_canvas_t NQCanvasPath;
typedef plutovg_line_cap_t NQCanvasLineCap;
typedef plutovg_line_join_t NQCanvasLineJoin;
typedef plutovg_font_face_cache_t NQCanvasFontFaceCache;
typedef plutovg_font_face_t NQCanvasFontFace;
typedef float NQCanvasFloat;

typedef struct NQCanvasFontMetrics {
  NQCanvasFloat ascent;
  NQCanvasFloat descent;
} NQCanvasFontMetrics;

/* A premultiplied ARGB surface the canvas paints from */
typedef plutovg_surface_t NQCanvasImage;

#define toCanvasFloat(value) ((float)(value))
#define getCanvasContext(thiz) (thiz)->canvas
#define getCanvasTextState(thiz) (&(thiz)->text)

static inline NQCanvasFontFaceCache* NQCanvasFontFaceCache_create(void)
{
  return plutovg_font_face_cache_create();
}

static inline void NQCanvasFontFaceCache_destroy(NQCanvasFontFaceCache* thiz)
{
  plutovg_font_face_cache_destroy(thiz);
}

static inline void NQCanvasFontFaceCache_loadSystem(NQCanvasFontFaceCache* thiz)
{
  plutovg_font_face_cache_load_sys(thiz);
}

static inline NQCanvasFontFace* NQCanvasFontFaceCache_get(NQCanvasFontFaceCache* thiz, const char* family, NQFontSlant slant, NQFontWeight weight)
{
  NQCanvasFontFace* fontFace;

  bool bold = (weight == NQ_FONT_WEIGHT_BOLD);
  bool italic = (slant != NQ_FONT_SLANT_NORMAL);

  fontFace = plutovg_font_face_cache_get(thiz, family, bold, italic);
  if (fontFace == NULL)
    return NULL;

  return plutovg_font_face_reference(fontFace);
}

static inline int NQCanvasImage_width(NQCanvasImage* image)
{
  return plutovg_surface_get_width(image);
}
#define NQCanvasImage_width NQCanvasImage_width

static inline int NQCanvasImage_height(NQCanvasImage* image)
{
  return plutovg_surface_get_height(image);
}
#define NQCanvasImage_height NQCanvasImage_height

static inline void NQCanvasContext_release(NQCanvasContext* thiz)
{
  plutovg_canvas_destroy(thiz);
}

static inline void NQCanvasContext_save(NQCanvasContext* thiz)
{
  plutovg_canvas_save(thiz);
}
#define NQCanvasContext_save NQCanvasContext_save

static inline void NQCanvasContext_restore(NQCanvasContext* thiz)
{
  plutovg_canvas_restore(thiz);
}
#define NQCanvasContext_restore NQCanvasContext_restore

static inline void NQCanvasContext_clipRect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  plutovg_canvas_clip_rect(thiz, x, y, w, h);
}
#define NQCanvasContext_clipRect NQCanvasContext_clipRect

static inline void NQCanvasContext_clip(NQCanvasContext* thiz)
{
  plutovg_canvas_clip(thiz);
}
#define NQCanvasContext_clip NQCanvasContext_clip

static inline void NQCanvasContext_translate(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  plutovg_canvas_translate(thiz, x, y);
}
#define NQCanvasContext_translate NQCanvasContext_translate

static inline void NQCanvasContext_scale(NQCanvasContext* thiz, NQCanvasFloat sx, NQCanvasFloat sy)
{
  plutovg_canvas_scale(thiz, sx, sy);
}
#define NQCanvasContext_scale NQCanvasContext_scale

static inline void NQCanvasContext_rotate(NQCanvasContext* thiz, NQCanvasFloat angle)
{
  plutovg_canvas_rotate(thiz, angle);
}
#define NQCanvasContext_rotate NQCanvasContext_rotate

static inline void NQCanvasContext_transform(NQCanvasContext* thiz, NQCanvasFloat a, NQCanvasFloat b, NQCanvasFloat c, NQCanvasFloat d, NQCanvasFloat e, NQCanvasFloat f)
{
  plutovg_matrix_t m;
  plutovg_matrix_init(&m, a, b, c, d, e, f);
  plutovg_canvas_transform(thiz, &m);
}
#define NQCanvasContext_transform NQCanvasContext_transform

static inline void NQCanvasContext_setTransform(NQCanvasContext* thiz, NQCanvasFloat a, NQCanvasFloat b, NQCanvasFloat c, NQCanvasFloat d, NQCanvasFloat e, NQCanvasFloat f)
{
  plutovg_matrix_t m;
  plutovg_matrix_init(&m, a, b, c, d, e, f);
  plutovg_canvas_set_matrix(thiz, &m);
}
#define NQCanvasContext_setTransform NQCanvasContext_setTransform

static inline void NQCanvasContext_getTransform(NQCanvasContext* thiz, double m[6])
{
  plutovg_matrix_t pm;
  plutovg_canvas_get_matrix(thiz, &pm);
  m[0] = pm.a;
  m[1] = pm.b;
  m[2] = pm.c;
  m[3] = pm.d;
  m[4] = pm.e;
  m[5] = pm.f;
}
#define NQCanvasContext_getTransform NQCanvasContext_getTransform

static inline void NQCanvasContext_setColor(NQCanvasContext* thiz, NQCanvasFloat r, NQCanvasFloat g, NQCanvasFloat b, NQCanvasFloat a)
{
  plutovg_canvas_set_rgba(thiz, r, g, b, a);
}
#define NQCanvasContext_setColor NQCanvasContext_setColor

static inline void NQCanvasContext_setLineWidth(NQCanvasContext* thiz, NQCanvasFloat v)
{
  plutovg_canvas_set_line_width(thiz, v);
}
#define NQCanvasContext_setLineWidth NQCanvasContext_setLineWidth

static inline NQCanvasLineCap toCanvasLineCap(NQLineCap lineCap)
{
  switch (lineCap) {
  case NQ_LINECAP_BUTT:
    return PLUTOVG_LINE_CAP_BUTT;
  case NQ_LINECAP_ROUND:
    return PLUTOVG_LINE_CAP_ROUND;
  case NQ_LINECAP_SQUARE:
    return PLUTOVG_LINE_CAP_SQUARE;
  }
  NQ_ASSERT_NOT_REACHED();
  return PLUTOVG_LINE_CAP_BUTT;
}

static inline void NQCanvasContext_setLineCap(NQCanvasContext* thiz, NQCanvasLineCap lineCap)
{
  plutovg_canvas_set_line_cap(thiz, lineCap);
}
#define NQCanvasContext_setLineCap NQCanvasContext_setLineCap

static inline NQCanvasLineJoin toCanvasLineJoin(NQLineJoin lineJoin)
{
  switch (lineJoin) {
  case NQ_LINEJOIN_MITER:
    return PLUTOVG_LINE_JOIN_MITER;
  case NQ_LINEJOIN_ROUND:
    return PLUTOVG_LINE_JOIN_ROUND;
  case NQ_LINEJOIN_BEVEL:
    return PLUTOVG_LINE_JOIN_BEVEL;
  }
  NQ_ASSERT_NOT_REACHED();
  return PLUTOVG_LINE_JOIN_MITER;
}

static inline void NQCanvasContext_setLineJoin(NQCanvasContext* thiz, NQCanvasLineJoin lineJoin)
{
  plutovg_canvas_set_line_join(thiz, lineJoin);
}
#define NQCanvasContext_setLineJoin NQCanvasContext_setLineJoin

static inline void NQCanvasContext_setMiterLimit(NQCanvasContext* thiz, NQCanvasFloat miterLimit)
{
  plutovg_canvas_set_miter_limit(thiz, miterLimit);
}
#define NQCanvasContext_setMiterLimit NQCanvasContext_setMiterLimit

static inline void NQCanvasContext_beginPath(NQCanvasContext* thiz)
{
  plutovg_canvas_new_path(thiz);
}
#define NQCanvasContext_beginPath NQCanvasContext_beginPath

static inline void NQCanvasContext_moveTo(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  plutovg_canvas_move_to(thiz, x, y);
}
#define NQCanvasContext_moveTo NQCanvasContext_moveTo

static inline void NQCanvasContext_lineTo(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  plutovg_canvas_line_to(thiz, x, y);
}
#define NQCanvasContext_lineTo NQCanvasContext_lineTo

static inline void NQCanvasContext_quadraticCurveTo(NQCanvasContext* thiz, NQCanvasFloat cpx, NQCanvasFloat cpy, NQCanvasFloat x, NQCanvasFloat y)
{
  plutovg_canvas_quad_to(thiz, cpx, cpy, x, y);
}
#define NQCanvasContext_quadraticCurveTo NQCanvasContext_quadraticCurveTo

static inline void NQCanvasContext_bezierCurveTo(NQCanvasContext* thiz, NQCanvasFloat cp1x, NQCanvasFloat cp1y, NQCanvasFloat cp2x, NQCanvasFloat cp2y, NQCanvasFloat x, NQCanvasFloat y)
{
  plutovg_canvas_cubic_to(thiz, cp1x, cp1y, cp2x, cp2y, x, y);
}
#define NQCanvasContext_bezierCurveTo NQCanvasContext_bezierCurveTo

static inline void NQCanvasContext_arc(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat radius, NQCanvasFloat startAngle, NQCanvasFloat endAngle)
{
  plutovg_canvas_arc(thiz, x, y, radius, startAngle, endAngle, false);
}
#define NQCanvasContext_arc NQCanvasContext_arc

static inline void NQCanvasContext_arcNegative(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat radius, NQCanvasFloat startAngle, NQCanvasFloat endAngle)
{
  plutovg_canvas_arc(thiz, x, y, radius, startAngle, endAngle, true);
}
#define NQCanvasContext_arcNegative NQCanvasContext_arcNegative

/* plutovg reports (0, 0) for an empty path, so look at the elements first */
static inline bool NQCanvasContext_currentPoint(NQCanvasContext* thiz, NQCanvasFloat* x, NQCanvasFloat* y)
{
  if (plutovg_path_get_elements(plutovg_canvas_get_path(thiz), NULL) == 0)
    return false;
  plutovg_canvas_get_current_point(thiz, x, y);
  return true;
}
#define NQCanvasContext_currentPoint NQCanvasContext_currentPoint

static inline void NQCanvasContext_rect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  plutovg_canvas_rect(thiz, x, y, w, h);
}
#define NQCanvasContext_rect NQCanvasContext_rect

static inline void NQCanvasContext_roundedRect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h, NQCanvasFloat r)
{
  plutovg_canvas_round_rect(thiz, x, y, w, h, r, r);
}
#define NQCanvasContext_roundedRect NQCanvasContext_roundedRect

static inline void NQCanvasContext_ellipse(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  plutovg_canvas_ellipse(thiz, x + w / 2, y + h / 2, w / 2, h / 2);
}
#define NQCanvasContext_ellipse NQCanvasContext_ellipse

static inline void NQCanvasContext_closePath(NQCanvasContext* thiz)
{
  plutovg_canvas_close_path(thiz);
}
#define NQCanvasContext_closePath NQCanvasContext_closePath

static inline void NQCanvasContext_stroke(NQCanvasContext* thiz)
{
  plutovg_canvas_stroke(thiz);
}

static inline void NQCanvasContext_fill(NQCanvasContext* thiz)
{
  plutovg_canvas_fill(thiz);
}

static inline void NQCanvasContext_fillRect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  plutovg_canvas_fill_rect(thiz, x, y, w, h);
}
#define NQCanvasContext_fillRect NQCanvasContext_fillRect

static inline void NQCanvasContext_strokeRect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  plutovg_canvas_stroke_rect(thiz, x, y, w, h);
}
#define NQCanvasContext_strokeRect NQCanvasContext_strokeRect

static inline NQCanvasFontFace* NQCanvasFontFace_fromFile(const char* filename, size_t index)
{
  return plutovg_font_face_load_from_file(filename, index);
}

static inline void NQCanvasFontFace_release(NQCanvasFontFace* thiz)
{
  plutovg_font_face_destroy(thiz);
}
#define NQCanvasFontFace_release NQCanvasFontFace_release

static inline void NQCanvasContext_setFontFace(NQCanvasContext* thiz, NQCanvasFontFace* fontFace)
{
  plutovg_canvas_set_font_face(thiz, fontFace);
}
#define NQCanvasContext_setFontFace NQCanvasContext_setFontFace

static inline void NQCanvasContext_setFontSize(NQCanvasContext* thiz, NQCanvasFloat fontSize)
{
  plutovg_canvas_set_font_size(thiz, fontSize);
}
#define NQCanvasContext_setFontSize NQCanvasContext_setFontSize

static inline void NQCanvasContext_fontMetrics(NQCanvasContext* thiz, NQCanvasFontMetrics* fontMetrics)
{
  fontMetrics->ascent = 0;
  fontMetrics->descent = 0;
  plutovg_canvas_font_metrics(thiz, &fontMetrics->ascent, &fontMetrics->descent, NULL, NULL);
}
#define NQCanvasContext_fontMetrics NQCanvasContext_fontMetrics

static inline NQCanvasFloat NQCanvasFontMetrics_ascent(const NQCanvasFontMetrics* thiz)
{
  return thiz->ascent;
}

static inline NQCanvasFloat NQCanvasFontMetrics_descent(const NQCanvasFontMetrics* thiz)
{
  return (0 - thiz->descent);
}

static inline NQCanvasFloat NQCanvasContext_textWidth(NQCanvasContext* thiz, const char* text, size_t length)
{
  return plutovg_canvas_text_extents(thiz, text, (int)length, PLUTOVG_TEXT_ENCODING_UTF8, NULL);
}
#define NQCanvasContext_textWidth NQCanvasContext_textWidth

/* Draw `length` bytes of text whose alphabetic baseline starts at (x, y) */
static inline void NQCanvasContext_strokeText(NQCanvasContext* thiz, const char* text, size_t length, NQCanvasFloat x, NQCanvasFloat y)
{
  plutovg_canvas_stroke_text(thiz, text, (int)length, PLUTOVG_TEXT_ENCODING_UTF8, x, y);
}
#define NQCanvasContext_strokeText NQCanvasContext_strokeText

static inline void NQCanvasContext_fillText(NQCanvasContext* thiz, const char* text, size_t length, NQCanvasFloat x, NQCanvasFloat y)
{
  plutovg_canvas_fill_text(thiz, text, (int)length, PLUTOVG_TEXT_ENCODING_UTF8, x, y);
}
#define NQCanvasContext_fillText NQCanvasContext_fillText

static inline NQCanvasImage* NQCanvasImage_createRGBA(const void* pixels, int width, int height, int stride)
{
  plutovg_surface_t* surface = plutovg_surface_create(width, height);
  if (!surface)
    return NULL;

  /* a row at a time, since the two strides may differ */
  unsigned char* dst = plutovg_surface_get_data(surface);
  const int dstStride = plutovg_surface_get_stride(surface);
  const unsigned char* src = (const unsigned char*)pixels;
  for (int y = 0; y < height; y++)
    plutovg_convert_rgba_to_argb(dst + (size_t)dstStride * (size_t)y, src + (size_t)stride * (size_t)y, width, 1, dstStride);

  return surface;
}
#define NQCanvasImage_createRGBA NQCanvasImage_createRGBA

static inline void NQCanvasImage_release(NQCanvasImage* thiz)
{
  plutovg_surface_destroy(thiz);
}
#define NQCanvasImage_release NQCanvasImage_release

static inline void NQCanvasContext_drawImage(NQCanvasContext* thiz, NQCanvasImage* image,
    NQCanvasFloat sx, NQCanvasFloat sy, NQCanvasFloat sw, NQCanvasFloat sh,
    NQCanvasFloat dx, NQCanvasFloat dy, NQCanvasFloat dw, NQCanvasFloat dh)
{
  /* maps image pixels to user space so the source rect lands on the destination */
  plutovg_matrix_t matrix;
  plutovg_matrix_init_translate(&matrix, dx, dy);
  plutovg_matrix_scale(&matrix, dw / sw, dh / sh);
  plutovg_matrix_translate(&matrix, -sx, -sy);

  /* filling replaces the current path, which plutovg_canvas_save does not
   * keep, so it is put back afterwards */
  plutovg_path_t* path = plutovg_path_clone(plutovg_canvas_get_path(thiz));

  plutovg_canvas_save(thiz); /* keeps the color paint */
  /* plutovg only reads the texture, though it takes it mutable */
  plutovg_canvas_set_texture(thiz, image, PLUTOVG_TEXTURE_TYPE_PLAIN, 1.0f, &matrix);
  plutovg_canvas_fill_rect(thiz, dx, dy, dw, dh);
  plutovg_canvas_restore(thiz);

  plutovg_canvas_new_path(thiz);
  plutovg_canvas_add_path(thiz, path);
  plutovg_path_destroy(path);
}
#define NQCanvasContext_drawImage NQCanvasContext_drawImage

#ifdef __cplusplus
}
#endif

#endif /* CANVAS_BACKEND_PLUTOVG */
#endif /* _LIBNETQ_GFX_PLUTOVG_CANVAS_H */
