/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_GFX_CAIRO_CANVAS_H
#define _LIBNETQ_GFX_CAIRO_CANVAS_H
#ifdef CANVAS_BACKEND_CAIRO

#include <libnetq/Assert.h>
#include <libnetq/gfx/GfxType.h>
#include <libnetq/gfx/ColorUtils.h>

#include <cairo.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_CANVAS_DEFINED_INTERNAL
#define NQ_CANVAS_BACKEND_NAME "cairo"

typedef struct NQCanvas NQCanvas;
typedef cairo_t NQCanvasContext;
typedef cairo_line_cap_t NQCanvasLineCap;
typedef cairo_line_join_t NQCanvasLineJoin;
typedef cairo_font_face_t NQCanvasFontFace;
typedef cairo_scaled_font_t NQCanvasFont;
typedef cairo_font_slant_t NQCanvasFontSlant;
typedef cairo_font_weight_t NQCanvasFontWeight;
typedef double NQCanvasFloat;
/* A premultiplied ARGB32 image surface the context paints from */
typedef cairo_surface_t NQCanvasImage;
typedef cairo_font_extents_t NQCanvasFontMetrics;

/* The context paints into the surface; both belong to the NQCanvas */
struct NQCanvas {
  cairo_surface_t* surface;
  cairo_t* canvas;
  NQTextState text;
};

/* Draws into `surface` of any cairo backend (an xlib pixmap, say), which
 * the canvas takes a reference to; NQCanvas_release finishes it. */
NQ_EXPORT NQCanvas* NQCanvas_fromCairoSurface(cairo_surface_t* surface);

#define toCanvasFloat(value) (value)
#define getCanvasContext(thiz) (thiz)->canvas
#define getCanvasTextState(thiz) (&(thiz)->text)

static inline NQCanvasFontSlant toCanvasFontSlant(NQFontSlant fontSlant)
{
  switch (fontSlant) {
  case NQ_FONT_SLANT_NORMAL:
    return CAIRO_FONT_SLANT_NORMAL;
  case NQ_FONT_SLANT_ITALIC:
    return CAIRO_FONT_SLANT_ITALIC;
  case NQ_FONT_SLANT_OBLIQUE:
    return CAIRO_FONT_SLANT_OBLIQUE;
  }
  NQ_ASSERT_NOT_REACHED();
  return CAIRO_FONT_SLANT_NORMAL;
}

static inline NQCanvasFontWeight toCanvasFontWeight(NQFontWeight fontWeight)
{
  switch (fontWeight) {
  case NQ_FONT_WEIGHT_NORMAL:
    return CAIRO_FONT_WEIGHT_NORMAL;
  case NQ_FONT_WEIGHT_BOLD:
    return CAIRO_FONT_WEIGHT_BOLD;
  }
  NQ_ASSERT_NOT_REACHED();
  return CAIRO_FONT_WEIGHT_NORMAL;
}

static inline NQCanvasFontFace* NQCanvasFontFace_create(const char* family, NQFontSlant slant, NQFontWeight weight)
{
  NQCanvasFontFace* fontFace = cairo_toy_font_face_create(family, toCanvasFontSlant(slant), toCanvasFontWeight(weight));
  if (cairo_font_face_status(fontFace) != CAIRO_STATUS_SUCCESS) {
    cairo_font_face_destroy(fontFace);
    return NULL;
  }
  return fontFace;
}

static inline int NQCanvasImage_width(NQCanvasImage* image)
{
  return cairo_image_surface_get_width(image);
}
#define NQCanvasImage_width NQCanvasImage_width

static inline int NQCanvasImage_height(NQCanvasImage* image)
{
  return cairo_image_surface_get_height(image);
}
#define NQCanvasImage_height NQCanvasImage_height

static inline void NQCanvasContext_release(NQCanvasContext* thiz)
{
  cairo_destroy(thiz);
}

static inline void NQCanvasContext_save(NQCanvasContext* thiz)
{
  cairo_save(thiz);
}
#define NQCanvasContext_save NQCanvasContext_save

static inline void NQCanvasContext_restore(NQCanvasContext* thiz)
{
  cairo_restore(thiz);
}
#define NQCanvasContext_restore NQCanvasContext_restore

static inline void NQCanvasContext_clip(NQCanvasContext* thiz)
{
  cairo_clip(thiz);
}
#define NQCanvasContext_clip NQCanvasContext_clip

static inline void NQCanvasContext_translate(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  cairo_translate(thiz, x, y);
}
#define NQCanvasContext_translate NQCanvasContext_translate

static inline void NQCanvasContext_scale(NQCanvasContext* thiz, NQCanvasFloat sx, NQCanvasFloat sy)
{
  cairo_scale(thiz, sx, sy);
}
#define NQCanvasContext_scale NQCanvasContext_scale

static inline void NQCanvasContext_rotate(NQCanvasContext* thiz, NQCanvasFloat angle)
{
  cairo_rotate(thiz, angle);
}
#define NQCanvasContext_rotate NQCanvasContext_rotate

/* A singular matrix puts a cairo context in an error state for good, so it
 * is not handed to cairo at all */
static inline bool toCanvasMatrix(cairo_matrix_t* m, NQCanvasFloat a, NQCanvasFloat b, NQCanvasFloat c, NQCanvasFloat d, NQCanvasFloat e, NQCanvasFloat f)
{
  cairo_matrix_init(m, a, b, c, d, e, f);
  cairo_matrix_t inverse = *m;
  return cairo_matrix_invert(&inverse) == CAIRO_STATUS_SUCCESS;
}

/* What a singular matrix would draw instead: nothing, until the restore
 * that undoes the clip. The current path is kept, as a transform keeps it. */
static inline void NQCanvasContext_clipAll(NQCanvasContext* thiz)
{
  cairo_path_t* path = cairo_copy_path(thiz);
  cairo_new_path(thiz);
  cairo_rectangle(thiz, 0, 0, 0, 0);
  cairo_clip(thiz);
  cairo_append_path(thiz, path);
  cairo_path_destroy(path);
}

static inline void NQCanvasContext_transform(NQCanvasContext* thiz, NQCanvasFloat a, NQCanvasFloat b, NQCanvasFloat c, NQCanvasFloat d, NQCanvasFloat e, NQCanvasFloat f)
{
  cairo_matrix_t m;
  if (toCanvasMatrix(&m, a, b, c, d, e, f))
    cairo_transform(thiz, &m);
  else
    NQCanvasContext_clipAll(thiz);
}
#define NQCanvasContext_transform NQCanvasContext_transform

static inline void NQCanvasContext_setTransform(NQCanvasContext* thiz, NQCanvasFloat a, NQCanvasFloat b, NQCanvasFloat c, NQCanvasFloat d, NQCanvasFloat e, NQCanvasFloat f)
{
  cairo_matrix_t m;
  if (toCanvasMatrix(&m, a, b, c, d, e, f))
    cairo_set_matrix(thiz, &m);
  else
    NQCanvasContext_clipAll(thiz);
}
#define NQCanvasContext_setTransform NQCanvasContext_setTransform

static inline void NQCanvasContext_getTransform(NQCanvasContext* thiz, double m[6])
{
  cairo_matrix_t cm;
  cairo_get_matrix(thiz, &cm);
  m[0] = cm.xx;
  m[1] = cm.yx;
  m[2] = cm.xy;
  m[3] = cm.yy;
  m[4] = cm.x0;
  m[5] = cm.y0;
}
#define NQCanvasContext_getTransform NQCanvasContext_getTransform

static inline void NQCanvasContext_setColor(NQCanvasContext* thiz, NQCanvasFloat r, NQCanvasFloat g, NQCanvasFloat b, NQCanvasFloat a)
{
  cairo_set_source_rgba(thiz, r, g, b, a);
}
#define NQCanvasContext_setColor NQCanvasContext_setColor

static inline void NQCanvasContext_setLineWidth(NQCanvasContext* thiz, NQCanvasFloat v)
{
  cairo_set_line_width(thiz, v);
}
#define NQCanvasContext_setLineWidth NQCanvasContext_setLineWidth

static inline NQCanvasLineCap toCanvasLineCap(NQLineCap lineCap)
{
  switch (lineCap) {
  case NQ_LINECAP_BUTT:
    return CAIRO_LINE_CAP_BUTT;
  case NQ_LINECAP_ROUND:
    return CAIRO_LINE_CAP_ROUND;
  case NQ_LINECAP_SQUARE:
    return CAIRO_LINE_CAP_SQUARE;
  }
  NQ_ASSERT_NOT_REACHED();
  return CAIRO_LINE_CAP_BUTT;
}

static inline void NQCanvasContext_setLineCap(NQCanvasContext* thiz, NQCanvasLineCap lineCap)
{
  cairo_set_line_cap(thiz, lineCap);
}
#define NQCanvasContext_setLineCap NQCanvasContext_setLineCap

static inline NQCanvasLineJoin toCanvasLineJoin(NQLineJoin lineJoin)
{
  switch (lineJoin) {
  case NQ_LINEJOIN_MITER:
    return CAIRO_LINE_JOIN_MITER;
  case NQ_LINEJOIN_ROUND:
    return CAIRO_LINE_JOIN_ROUND;
  case NQ_LINEJOIN_BEVEL:
    return CAIRO_LINE_JOIN_BEVEL;
  }
  NQ_ASSERT_NOT_REACHED();
  return CAIRO_LINE_JOIN_MITER;
}

static inline void NQCanvasContext_setLineJoin(NQCanvasContext* thiz, NQCanvasLineJoin lineJoin)
{
  cairo_set_line_join(thiz, lineJoin);
}
#define NQCanvasContext_setLineJoin NQCanvasContext_setLineJoin

static inline void NQCanvasContext_setMiterLimit(NQCanvasContext* thiz, NQCanvasFloat miterLimit)
{
  cairo_set_miter_limit(thiz, miterLimit);
}
#define NQCanvasContext_setMiterLimit NQCanvasContext_setMiterLimit

static inline void NQCanvasContext_beginPath(NQCanvasContext* thiz)
{
  cairo_new_path(thiz);
}
#define NQCanvasContext_beginPath NQCanvasContext_beginPath

static inline void NQCanvasContext_moveTo(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  cairo_move_to(thiz, x, y);
}
#define NQCanvasContext_moveTo NQCanvasContext_moveTo

static inline void NQCanvasContext_lineTo(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  cairo_line_to(thiz, x, y);
}
#define NQCanvasContext_lineTo NQCanvasContext_lineTo

static inline void NQCanvasContext_bezierCurveTo(NQCanvasContext* thiz, NQCanvasFloat cp1x, NQCanvasFloat cp1y, NQCanvasFloat cp2x, NQCanvasFloat cp2y, NQCanvasFloat x, NQCanvasFloat y)
{
  cairo_curve_to(thiz, cp1x, cp1y, cp2x, cp2y, x, y);
}
#define NQCanvasContext_bezierCurveTo NQCanvasContext_bezierCurveTo

static inline void NQCanvasContext_arc(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat radius, NQCanvasFloat startAngle, NQCanvasFloat endAngle)
{
  cairo_arc(thiz, x, y, radius, startAngle, endAngle);
}
#define NQCanvasContext_arc NQCanvasContext_arc

static inline void NQCanvasContext_arcNegative(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat radius, NQCanvasFloat startAngle, NQCanvasFloat endAngle)
{
  cairo_arc_negative(thiz, x, y, radius, startAngle, endAngle);
}
#define NQCanvasContext_arcNegative NQCanvasContext_arcNegative

/* In user space, like the coordinates the path calls take */
static inline bool NQCanvasContext_currentPoint(NQCanvasContext* thiz, NQCanvasFloat* x, NQCanvasFloat* y)
{
  if (!cairo_has_current_point(thiz))
    return false;
  cairo_get_current_point(thiz, x, y);
  return true;
}
#define NQCanvasContext_currentPoint NQCanvasContext_currentPoint

static inline void NQCanvasContext_rect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  cairo_rectangle(thiz, x, y, w, h);
}
#define NQCanvasContext_rect NQCanvasContext_rect

static inline void NQCanvasContext_closePath(NQCanvasContext* thiz)
{
  cairo_close_path(thiz);
}
#define NQCanvasContext_closePath NQCanvasContext_closePath

static inline void NQCanvasContext_stroke(NQCanvasContext* thiz)
{
  cairo_stroke(thiz);
}

static inline void NQCanvasContext_fill(NQCanvasContext* thiz)
{
  cairo_fill(thiz);
}

static inline void NQCanvasFontFace_release(NQCanvasFontFace* thiz)
{
  cairo_font_face_destroy(thiz);
}
#define NQCanvasFontFace_release NQCanvasFontFace_release

static inline void NQCanvasContext_setFontFace(NQCanvasContext* thiz, NQCanvasFontFace* fontFace)
{
  cairo_set_font_face(thiz, fontFace);
}
#define NQCanvasContext_setFontFace NQCanvasContext_setFontFace

static inline void NQCanvasContext_setFontSize(NQCanvasContext* thiz, NQCanvasFloat fontSize)
{
  cairo_set_font_size(thiz, fontSize);
}
#define NQCanvasContext_setFontSize NQCanvasContext_setFontSize

static inline void NQCanvasContext_fontMetrics(NQCanvasContext* thiz, NQCanvasFontMetrics* fontMetrics)
{
  cairo_font_extents(thiz, fontMetrics);
}
#define NQCanvasContext_fontMetrics NQCanvasContext_fontMetrics

static inline NQCanvasFloat NQCanvasFontMetrics_ascent(const NQCanvasFontMetrics* thiz)
{
  return thiz->ascent;
}

static inline NQCanvasFloat NQCanvasFontMetrics_descent(const NQCanvasFontMetrics* thiz)
{
  return thiz->descent;
}

static inline NQCanvasFloat NQCanvasContext_textWidth(NQCanvasContext* thiz, const char* text, size_t length)
{
  cairo_status_t status;
  cairo_glyph_t* glyphs = NULL;
  int numGlyphs = 0;
  NQCanvasFont* scaledFont = cairo_get_scaled_font(thiz);

  status = cairo_scaled_font_text_to_glyphs(scaledFont, 0, 0, text, (int)length, &glyphs, &numGlyphs, NULL, NULL, NULL);
  if (status != CAIRO_STATUS_SUCCESS)
    return 0;

  cairo_text_extents_t ext;
  cairo_glyph_extents(thiz, glyphs, numGlyphs, &ext);
  cairo_glyph_free(glyphs);
  return ext.x_advance;
}
#define NQCanvasContext_textWidth NQCanvasContext_textWidth

/* Lands every pending write in the pixels before someone else reads them */
static inline void NQCanvasImpl_flush(NQCanvas* thiz)
{
  cairo_surface_flush(thiz->surface);
}
#define NQCanvasImpl_flush NQCanvasImpl_flush

void NQCanvasContext_strokeText(NQCanvasContext*, const char* text, size_t length, NQCanvasFloat x, NQCanvasFloat y);
#define NQCanvasContext_strokeText NQCanvasContext_strokeText
void NQCanvasContext_fillText(NQCanvasContext*, const char* text, size_t length, NQCanvasFloat x, NQCanvasFloat y);
#define NQCanvasContext_fillText NQCanvasContext_fillText

static inline NQCanvasImage* NQCanvasImage_createRGBA(const void* pixels, int width, int height, int stride)
{
  cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
    cairo_surface_destroy(surface);
    return NULL;
  }

  /* cairo wants native-endian premultiplied 0xAARRGGBB words */
  cairo_surface_flush(surface);
  unsigned char* dst = cairo_image_surface_get_data(surface);
  const int dstStride = cairo_image_surface_get_stride(surface);
  const unsigned char* src = (const unsigned char*)pixels;
  for (int y = 0; y < height; y++) {
    const unsigned char* in = src + (size_t)stride * (size_t)y;
    uint32_t* out = (uint32_t*)(dst + (size_t)dstStride * (size_t)y);
    NQRgbaToPremultipliedArgbLine((uint32_t)width, in, out);
  }
  cairo_surface_mark_dirty(surface);
  return surface;
}
#define NQCanvasImage_createRGBA NQCanvasImage_createRGBA

static inline void NQCanvasImage_release(NQCanvasImage* thiz)
{
  cairo_surface_destroy(thiz);
}
#define NQCanvasImage_release NQCanvasImage_release

static inline void NQCanvasContext_drawImage(NQCanvasContext* thiz, NQCanvasImage* image,
    NQCanvasFloat sx, NQCanvasFloat sy, NQCanvasFloat sw, NQCanvasFloat sh,
    NQCanvasFloat dx, NQCanvasFloat dy, NQCanvasFloat dw, NQCanvasFloat dh)
{
  /* the clip goes through the path, which cairo_save does not keep */
  cairo_path_t* path = cairo_copy_path(thiz);
  cairo_save(thiz); /* keeps the color source */
  cairo_new_path(thiz);
  cairo_rectangle(thiz, dx, dy, dw, dh);
  cairo_clip(thiz);
  /* image pixels to user space, so the source rect lands on the destination */
  cairo_translate(thiz, dx, dy);
  cairo_scale(thiz, dw / sw, dh / sh);
  cairo_set_source_surface(thiz, image, -sx, -sy);
  /* filtering past the image's edge repeats the edge pixels, as the HTML
   * canvas does; EXTEND_NONE would fade a scaled image's border out */
  cairo_pattern_set_extend(cairo_get_source(thiz), CAIRO_EXTEND_PAD);
  cairo_paint(thiz);
  cairo_restore(thiz);
  cairo_append_path(thiz, path);
  cairo_path_destroy(path);
}
#define NQCanvasContext_drawImage NQCanvasContext_drawImage

#ifdef __cplusplus
}
#endif

#endif /* CANVAS_BACKEND_CAIRO */
#endif /* _LIBNETQ_GFX_CAIRO_CANVAS_H */
