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

#include <plutovg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_CANVAS_BACKEND_NAME "plutovg"

typedef plutovg_canvas_t NQCanvas;
typedef plutovg_canvas_t NQCanvasContext;
typedef plutovg_canvas_t NQCanvasPaint;
typedef plutovg_canvas_t NQCanvasPath;
typedef plutovg_line_cap_t NQCanvasLineCap;
typedef float NQCanvasFloat;
typedef plutovg_font_face_t NQGFont;

#define toCanvasFloat(value) ((float)(value))
#define toCanvasContext(canvas) (canvas)
#define toCanvasPaint(canvas) (canvas)
#define toCanvasPath(canvas) (canvas)

static inline void NQCanvasContext_release(NQCanvasContext* thiz)
{
  plutovg_canvas_destroy(thiz);
}

static inline void NQCanvasContext_save(NQCanvasContext* thiz)
{
  plutovg_canvas_save(thiz);
}

static inline void NQCanvasContext_restore(NQCanvasContext* thiz)
{
  plutovg_canvas_restore(thiz);
}

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

static inline void NQCanvasContext_scale(NQCanvasContext* thiz, NQCanvasFloat sx, NQCanvasFloat sy)
{
  plutovg_canvas_scale(thiz, sx, sy);
}

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

static inline plutovg_line_join_t toBackenLineJoin(NQLineJoin lineJoin)
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

static inline void NQCanvasPaint_setLineJoin(NQCanvasPaint* thiz, plutovg_line_join_t lineJoin)
{
  plutovg_canvas_set_line_join(thiz, lineJoin);
}

static inline void NQCanvasPaint_setMiterLimit(NQCanvasPath* thiz, NQCanvasFloat miterLimit)
{
  plutovg_canvas_set_miter_limit(thiz, miterLimit);
}

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

static inline void NQCanvasContext_arc(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat radius, NQCanvasFloat startAngle, NQCanvasFloat endAngle)
{
  plutovg_canvas_arc(thiz, x, y, radius, startAngle, endAngle, false);
}
#define NQCanvasContext_arc NQCanvasContext_arc

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

static inline void NQCanvasPath_ellipse(NQCanvasPath* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  plutovg_canvas_ellipse(thiz, x + w / 2, y + h / 2, w / 2, h / 2);
}
#define NQCanvasPath_ellipse NQCanvasPath_ellipse

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

#ifdef __cplusplus
}
#endif

#endif /* CANVAS_BACKEND_PLUTOVG */
#endif /* _LIBNETQ_GFX_PLUTOVG_CANVAS_H */
