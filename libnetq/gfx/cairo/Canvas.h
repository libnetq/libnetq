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

#include <cairo.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_CANVAS_BACKEND_NAME "cairo"

typedef struct NQCanvas NQCanvas;
typedef cairo_t NQCanvasContext;
typedef NQCanvasContext NQCanvasPaint;
typedef cairo_line_cap_t NQCanvasLineCap;
typedef double NQCanvasFloat;
typedef cairo_font_face_t NQGFont;

/* cairo_t is opaque, so the canvas holds a reference to one */
struct NQCanvas {
  cairo_t* canvas;
};

#define toCanvasFloat(value) (value)
#define toCanvasContext(thiz) (thiz)->canvas
#define toCanvasPaint(thiz) (thiz)->canvas

static inline void NQCanvasContext_release(NQCanvasContext* thiz)
{
  cairo_destroy(thiz);
}

static inline void NQCanvasContext_save(NQCanvasContext* thiz)
{
  cairo_save(thiz);
}

static inline void NQCanvasContext_restore(NQCanvasContext* thiz)
{
  cairo_restore(thiz);
}

static inline void NQCanvasContext_clip(NQCanvasContext* thiz)
{
  cairo_clip(thiz);
}
#define NQCanvasContext_clip NQCanvasContext_clip

static inline void NQCanvasContext_translate(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  cairo_translate(thiz, x, y);
}

static inline void NQCanvasContext_scale(NQCanvasContext* thiz, NQCanvasFloat sx, NQCanvasFloat sy)
{
  cairo_scale(thiz, sx, sy);
}

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

static inline cairo_line_join_t toBackenLineJoin(NQLineJoin lineJoin)
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

static inline void NQCanvasPaint_setLineJoin(NQCanvasPaint* thiz, cairo_line_join_t lineJoin)
{
  cairo_set_line_join(thiz, lineJoin);
}

static inline void NQCanvasPaint_setMiterLimit(NQCanvasPaint* thiz, NQCanvasFloat miterLimit)
{
  cairo_set_miter_limit(thiz, miterLimit);
}

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

static inline void NQCanvasContext_arc(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat radius, NQCanvasFloat startAngle, NQCanvasFloat endAngle)
{
  cairo_arc(thiz, x, y, radius, startAngle, endAngle);
}
#define NQCanvasContext_arc NQCanvasContext_arc

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

#ifdef __cplusplus
}
#endif

#endif /* CANVAS_BACKEND_CAIRO */
#endif /* _LIBNETQ_GFX_CAIRO_CANVAS_H */
