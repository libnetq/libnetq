/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_GFX_NANOVG_CANVAS_H
#define _LIBNETQ_GFX_NANOVG_CANVAS_H
#ifdef CANVAS_BACKEND_NANOVG

#include <libnetq/Assert.h>

#include <nanovg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_CANVAS_BACKEND_NAME "nanovg"

typedef NVGcontext NQCanvas;
typedef NVGcontext NQCanvasContext;
typedef NVGcontext NQCanvasPaint;
typedef NVGcontext NQCanvasPath;
typedef int NQCanvasLineCap;
typedef float NQCanvasFloat;
typedef void NQGFont;

#define toCanvasFloat(value) ((float)(value))
#define toCanvasContext(canvas) (canvas)
#define toCanvasPaint(canvas) (canvas)
#define toCanvasPath(canvas) (canvas)

static inline void NQCanvasContext_release(NQCanvasContext* thiz)
{
  nvgDeleteInternal(thiz);
}

static inline void NQCanvasContext_save(NQCanvasContext* thiz)
{
  nvgSave(thiz);
}

static inline void NQCanvasContext_restore(NQCanvasContext* thiz)
{
  nvgRestore(thiz);
}

static inline void NQCanvasContext_clipRect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  nvgIntersectScissor(thiz, x, y, w, h);
}
#define NQCanvasContext_clipRect NQCanvasContext_clipRect

static inline void NQCanvasContext_translate(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  nvgTranslate(thiz, x, y);
}

static inline void NQCanvasContext_scale(NQCanvasContext* thiz, NQCanvasFloat sx, NQCanvasFloat sy)
{
  nvgScale(thiz, sx, sy);
}

static inline void NQCanvasContext_setColor(NQCanvasContext* thiz, NQCanvasFloat r, NQCanvasFloat g, NQCanvasFloat b, NQCanvasFloat a)
{
  const NVGcolor c = nvgRGBAf(r, g, b, a);
  nvgFillColor(thiz, c);
  nvgStrokeColor(thiz, c);
}
#define NQCanvasContext_setColor NQCanvasContext_setColor

static inline void NQCanvasContext_setLineWidth(NQCanvasContext* thiz, NQCanvasFloat v)
{
  nvgStrokeWidth(thiz, v);
}
#define NQCanvasContext_setLineWidth NQCanvasContext_setLineWidth

static inline NQCanvasLineCap toCanvasLineCap(NQLineCap lineCap)
{
  switch (lineCap) {
  case NQ_LINECAP_BUTT:
    return NVG_BUTT;
  case NQ_LINECAP_ROUND:
    return NVG_ROUND;
  case NQ_LINECAP_SQUARE:
    return NVG_SQUARE;
  }
  NQ_ASSERT_NOT_REACHED();
  return NVG_BUTT;
}

static inline void NQCanvasContext_setLineCap(NQCanvasContext* thiz, NQCanvasLineCap lineCap)
{
  nvgLineCap(thiz, lineCap);
}
#define NQCanvasContext_setLineCap NQCanvasContext_setLineCap

static inline int toBackenLineJoin(NQLineJoin lineJoin)
{
  switch (lineJoin) {
  case NQ_LINEJOIN_MITER:
    return NVG_MITER;
  case NQ_LINEJOIN_ROUND:
    return NVG_ROUND;
  case NQ_LINEJOIN_BEVEL:
    return NVG_BEVEL;
  }
  NQ_ASSERT_NOT_REACHED();
  return NVG_MITER;
}

static inline void NQCanvasPaint_setLineJoin(NQCanvasPaint* thiz, int lineJoin)
{
  nvgLineJoin(thiz, lineJoin);
}

static inline void NQCanvasPaint_setMiterLimit(NQCanvasPaint* thiz, NQCanvasFloat miterLimit)
{
  nvgMiterLimit(thiz, miterLimit);
}

static inline void NQCanvasContext_beginPath(NQCanvasContext* thiz)
{
  nvgBeginPath(thiz);
}
#define NQCanvasContext_beginPath NQCanvasContext_beginPath

static inline void NQCanvasContext_moveTo(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  nvgMoveTo(thiz, x, y);
}
#define NQCanvasContext_moveTo NQCanvasContext_moveTo

static inline void NQCanvasContext_lineTo(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  nvgLineTo(thiz, x, y);
}
#define NQCanvasContext_lineTo NQCanvasContext_lineTo

static inline void NQCanvasContext_arc(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat radius, NQCanvasFloat startAngle, NQCanvasFloat endAngle)
{
  nvgArc(thiz, x, y, radius, startAngle, endAngle, NVG_CW);
}
#define NQCanvasContext_arc NQCanvasContext_arc

static inline void NQCanvasContext_rect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  nvgRect(thiz, x, y, w, h);
}
#define NQCanvasContext_rect NQCanvasContext_rect

static inline void NQCanvasContext_roundedRect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h, NQCanvasFloat r)
{
  nvgRoundedRect(thiz, x, y, w, h, r);
}
#define NQCanvasContext_roundedRect NQCanvasContext_roundedRect

static inline void NQCanvasPath_ellipse(NQCanvasPath* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  nvgEllipse(thiz, x + w / 2, y + h / 2, w / 2, h / 2);
}
#define NQCanvasPath_ellipse NQCanvasPath_ellipse

static inline void NQCanvasContext_closePath(NQCanvasContext* thiz)
{
  nvgClosePath(thiz);
}
#define NQCanvasContext_closePath NQCanvasContext_closePath

static inline void NQCanvasContext_stroke(NQCanvasContext* thiz)
{
  nvgStroke(thiz);
}

static inline void NQCanvasContext_fill(NQCanvasContext* thiz)
{
  nvgFill(thiz);
}

#ifdef __cplusplus
}
#endif

#endif /* CANVAS_BACKEND_NANOVG */
#endif /* _LIBNETQ_GFX_NANOVG_CANVAS_H */
