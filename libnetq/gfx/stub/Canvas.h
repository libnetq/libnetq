/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_GFX_STUB_CANVAS_H
#define _LIBNETQ_GFX_STUB_CANVAS_H

#ifdef CANVAS_BACKEND_STUB

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_CANVAS_BACKEND_NAME NULL

typedef void NQCanvas;
typedef void NQCanvasContext;
typedef void NQCanvasPaint;
typedef void NQCanvasPath;
typedef double NQCanvasFloat;
typedef struct NQGFont NQGFont;
typedef NQLineCap NQCanvasLineCap;

#define toCanvasFloat(value) (value)
#define toCanvasContext(canvas) (canvas)
#define toCanvasPaint(canvas) (canvas)
#define toCanvasPath(canvas) (canvas)

static inline void NQCanvasContext_release(NQCanvasContext* thiz)
{
  NQ_UNUSED_PARAM(thiz);
}

static inline void NQCanvasContext_save(NQCanvasContext* thiz)
{
  NQ_UNUSED_PARAM(thiz);
}

static inline void NQCanvasContext_restore(NQCanvasContext* thiz)
{
  NQ_UNUSED_PARAM(thiz);
}

static inline void NQCanvasContext_translate(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(x);
  NQ_UNUSED_PARAM(y);
}

static inline void NQCanvasContext_scale(NQCanvasContext* thiz, NQCanvasFloat sx, NQCanvasFloat sy)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(sx);
  NQ_UNUSED_PARAM(sy);
}

static inline void NQCanvasContext_setColor(NQCanvasContext* thiz, NQCanvasFloat r, NQCanvasFloat g, NQCanvasFloat b, NQCanvasFloat a)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(r);
  NQ_UNUSED_PARAM(g);
  NQ_UNUSED_PARAM(b);
  NQ_UNUSED_PARAM(a);
}
#define NQCanvasContext_setColor NQCanvasContext_setColor

static inline void NQCanvasContext_setLineWidth(NQCanvasContext* thiz, NQCanvasFloat lineWidth)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(lineWidth);
}
#define NQCanvasContext_setLineWidth NQCanvasContext_setLineWidth

#define toCanvasLineCap(lineCap) (lineCap)

static inline void NQCanvasContext_setLineCap(NQCanvasContext* thiz, NQCanvasLineCap lineCap)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(lineCap);
}
#define NQCanvasContext_setLineCap NQCanvasContext_setLineCap

#define toBackenLineJoin(lineJoin) (lineJoin)

static inline void NQCanvasPaint_setLineJoin(NQCanvasPaint* thiz, NQLineJoin lineJoin)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(lineJoin);
}

static inline void NQCanvasPaint_setMiterLimit(NQCanvasPaint* thiz, NQCanvasFloat miterLimit)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(miterLimit);
}

static inline void NQCanvasContext_beginPath(NQCanvasContext* thiz)
{
  NQ_UNUSED_PARAM(thiz);
}
#define NQCanvasContext_beginPath NQCanvasContext_beginPath

static inline void NQCanvasContext_moveTo(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(x);
  NQ_UNUSED_PARAM(y);
}
#define NQCanvasContext_moveTo NQCanvasContext_moveTo

static inline void NQCanvasContext_lineTo(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(x);
  NQ_UNUSED_PARAM(y);
}
#define NQCanvasContext_lineTo NQCanvasContext_lineTo

static inline void NQCanvasContext_arc(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat radius, NQCanvasFloat startAngle, NQCanvasFloat endAngle)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(x);
  NQ_UNUSED_PARAM(y);
  NQ_UNUSED_PARAM(radius);
  NQ_UNUSED_PARAM(startAngle);
  NQ_UNUSED_PARAM(endAngle);
}
#define NQCanvasContext_arc NQCanvasContext_arc

static inline void NQCanvasContext_closePath(NQCanvasContext* thiz)
{
  NQ_UNUSED_PARAM(thiz);
}
#define NQCanvasContext_closePath NQCanvasContext_closePath

static inline void NQCanvasContext_stroke(NQCanvasContext* thiz)
{
  NQ_UNUSED_PARAM(thiz);
}

static inline void NQCanvasContext_fill(NQCanvasContext* thiz)
{
  NQ_UNUSED_PARAM(thiz);
}

#ifdef __cplusplus
}
#endif

#endif /* CANVAS_BACKEND_STUB */
#endif /* _LIBNETQ_GFX_STUB_CANVAS_H */
