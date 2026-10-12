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

#include <libnetq/gfx/GfxType.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_CANVAS_DEFINED_INTERNAL
#define NQ_CANVAS_BACKEND_NAME NULL

typedef void NQCanvas;
typedef void NQCanvasContext;
typedef void NQCanvasFontFace;
typedef void NQCanvasImage;
typedef NQLineCap NQCanvasLineCap;
typedef NQLineJoin NQCanvasLineJoin;
typedef long NQCanvasFontMetrics;
typedef double NQCanvasFloat;

#define toCanvasFloat(value) (value)
#define getCanvasContext(canvas) (canvas)
#define toCanvasLineCap(lineCap) (lineCap)
#define toCanvasLineJoin(lineJoin) (lineJoin)

static inline void NQCanvasContext_release(NQCanvasContext* thiz)
{
  NQ_UNUSED_PARAM(thiz);
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

static inline void NQCanvasContext_quadraticCurveTo(NQCanvasContext* thiz, NQCanvasFloat cpx, NQCanvasFloat cpy, NQCanvasFloat x, NQCanvasFloat y)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(cpx);
  NQ_UNUSED_PARAM(cpy);
  NQ_UNUSED_PARAM(x);
  NQ_UNUSED_PARAM(y);
}
#define NQCanvasContext_quadraticCurveTo NQCanvasContext_quadraticCurveTo

static inline void NQCanvasContext_bezierCurveTo(NQCanvasContext* thiz, NQCanvasFloat cp1x, NQCanvasFloat cp1y, NQCanvasFloat cp2x, NQCanvasFloat cp2y, NQCanvasFloat x, NQCanvasFloat y)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(cp1x);
  NQ_UNUSED_PARAM(cp1y);
  NQ_UNUSED_PARAM(cp2x);
  NQ_UNUSED_PARAM(cp2y);
  NQ_UNUSED_PARAM(x);
  NQ_UNUSED_PARAM(y);
}
#define NQCanvasContext_bezierCurveTo NQCanvasContext_bezierCurveTo

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

static inline void NQCanvasContext_arcTo(NQCanvasContext* thiz, NQCanvasFloat x1, NQCanvasFloat y1, NQCanvasFloat x2, NQCanvasFloat y2, NQCanvasFloat radius)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(x1);
  NQ_UNUSED_PARAM(y1);
  NQ_UNUSED_PARAM(x2);
  NQ_UNUSED_PARAM(y2);
  NQ_UNUSED_PARAM(radius);
}
#define NQCanvasContext_arcTo NQCanvasContext_arcTo

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

static inline NQCanvasFloat NQCanvasFontMetrics_ascent(const NQCanvasFontMetrics* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return 0;
}

static inline NQCanvasFloat NQCanvasFontMetrics_descent(const NQCanvasFontMetrics* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* CANVAS_BACKEND_STUB */
#endif /* _LIBNETQ_GFX_STUB_CANVAS_H */
