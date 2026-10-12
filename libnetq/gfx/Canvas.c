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

#ifdef ENABLE_CANVAS

#ifdef CANVAS_BACKEND_NANOVG
#include "libnetq/gfx/nanovg/Canvas.h"
#endif

#ifdef CANVAS_BACKEND_PLUTOVG
#include "libnetq/gfx/plutovg/Canvas.h"
#endif

#ifdef CANVAS_BACKEND_CAIRO
#include "libnetq/gfx/cairo/Canvas.h"
#endif

#ifdef CANVAS_BACKEND_QUARTZ
#include "libnetq/gfx/quartz/Canvas.h"
#endif

#ifdef CANVAS_BACKEND_SKIA
#include "libnetq/gfx/skia/Canvas.h"
#endif

#ifdef CANVAS_BACKEND_STUB
#include "libnetq/gfx/stub/Canvas.h"
#endif

#include <libnetq/Math.h>

static inline void NQCanvasImpl_setColor(NQCanvas* thiz, NQCanvasFloat r, NQCanvasFloat g, NQCanvasFloat b, NQCanvasFloat a)
{
#if defined(NQCanvasContext_setColor)
  NQCanvasContext_setColor(toCanvasContext(thiz), r, g, b, a);
#elif defined(NQCanvasPaint_setColor)
  NQCanvasPaint_setColor(toCanvasPaint(thiz), r, g, b, a);
#else
# error "Canvas backend must define NQCanvasContext_setColor or NQCanvasPaint_setColor"
#endif
}

static inline void NQCanvasImpl_setLineCap(NQCanvas* thiz, NQCanvasLineCap lineCap)
{
#if defined(NQCanvasContext_setLineCap)
  NQCanvasContext_setLineCap(toCanvasContext(thiz), lineCap);
#elif defined(NQCanvasPaint_setLineCap)
  NQCanvasPaint_setLineCap(toCanvasPaint(thiz), lineCap);
#else
# error "Canvas backend must define NQCanvasContext_setLineCap or NQCanvasPaint_setLineCap"
#endif
}

static inline void NQCanvasImpl_setLineWidth(NQCanvas* thiz, NQCanvasFloat lineWidth)
{
#if defined(NQCanvasContext_setLineWidth)
  NQCanvasContext_setLineWidth(toCanvasContext(thiz), lineWidth);
#elif defined(NQCanvasPaint_setLineWidth)
  NQCanvasPaint_setLineWidth(toCanvasPaint(thiz), lineWidth);
#else
# error "Canvas backend must define NQCanvasContext_setLineWidth or NQCanvasPaint_setLineWidth"
#endif
}

static inline void NQCanvasImpl_beginPath(NQCanvas* thiz)
{
#if defined(NQCanvasContext_beginPath)
  NQCanvasContext_beginPath(toCanvasContext(thiz));
#elif defined(NQCanvasPath_beginPath)
  NQCanvasPath_beginPath(toCanvasPath(thiz));
#else
# error "Canvas backend must define NQCanvasContext_beginPath or NQCanvasPath_beginPath"
#endif
}

static inline void NQCanvasImpl_moveTo(NQCanvas* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
#if defined(NQCanvasContext_moveTo)
  NQCanvasContext_moveTo(toCanvasContext(thiz), x, y);
#elif defined(NQCanvasPath_moveTo)
  NQCanvasPath_moveTo(toCanvasPath(thiz), x, y);
#else
# error "Canvas backend must define NQCanvasContext_moveTo or NQCanvasPath_moveTo"
#endif
}

static inline void NQCanvasImpl_lineTo(NQCanvas* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
#if defined(NQCanvasContext_lineTo)
  NQCanvasContext_lineTo(toCanvasContext(thiz), x, y);
#elif defined(NQCanvasPath_lineTo)
  NQCanvasPath_lineTo(toCanvasPath(thiz), x, y);
#else
# error "Canvas backend must define NQCanvasContext_lineTo or NQCanvasPath_lineTo"
#endif
}

static inline void NQCanvasImpl_arc(NQCanvas* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat radius, NQCanvasFloat startAngle, NQCanvasFloat endAngle)
{
#if defined(NQCanvasContext_arc)
  NQCanvasContext_arc(toCanvasContext(thiz), x, y, radius, startAngle, endAngle);
#elif defined(NQCanvasPath_arc)
  NQCanvasPath_arc(toCanvasPath(thiz), x, y, radius, startAngle, endAngle);
#else
# error "Canvas backend must define NQCanvasContext_arc or NQCanvasPath_arc"
#endif
}

static inline void NQCanvasImpl_closePath(NQCanvas* thiz)
{
#if defined(NQCanvasContext_closePath)
  NQCanvasContext_closePath(toCanvasContext(thiz));
#elif defined(NQCanvasPath_closePath)
  NQCanvasPath_closePath(toCanvasPath(thiz));
#else
# error "Canvas backend must define NQCanvasContext_closePath or NQCanvasPath_closePath"
#endif
}

static inline void NQCanvasImpl_rect(NQCanvas* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
#if defined(NQCanvasContext_rect)
  NQCanvasContext_rect(toCanvasContext(thiz), x, y, w, h);
#elif defined(NQCanvasPath_rect)
  NQCanvasPath_rect(toCanvasPath(thiz), x, y, w, h);
#else
  NQCanvasImpl_moveTo(thiz, x, y);
  NQCanvasImpl_lineTo(thiz, x + w, y);
  NQCanvasImpl_lineTo(thiz, x + w, y + h);
  NQCanvasImpl_lineTo(thiz, x, y + h);
  NQCanvasImpl_closePath(thiz);
#endif
}

static inline void NQCanvasImpl_roundedRect(NQCanvas* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h, NQCanvasFloat r)
{
#if defined(NQCanvasContext_roundedRect)
  NQCanvasContext_roundedRect(toCanvasContext(thiz), x, y, w, h, r);
#elif defined(NQCanvasPath_roundedRect)
  NQCanvasPath_roundedRect(toCanvasPath(thiz), x, y, w, h, r);
#else
  if (r > w / 2)
    r = w / 2;
  if (r > h / 2)
    r = h / 2;
  /* a new subpath: the first arc's lead-in line is the top edge */
  NQCanvasImpl_moveTo(thiz, x + r, y);
  NQCanvasImpl_arc(thiz, x + w - r, y + r, r, -M_PI / 2, 0);
  NQCanvasImpl_arc(thiz, x + w - r, y + h - r, r, 0, M_PI / 2);
  NQCanvasImpl_arc(thiz, x + r, y + h - r, r, M_PI / 2, M_PI);
  NQCanvasImpl_arc(thiz, x + r, y + r, r, M_PI, 3 * M_PI / 2);
  NQCanvasImpl_closePath(thiz);
#endif
}

static inline void NQCanvasImpl_ellipse(NQCanvas* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
#ifdef NQCanvasPath_ellipse
  NQCanvasPath_ellipse(toCanvasPath(thiz), x, y, w, h);
#else
  /* a zero scale is a singular matrix, which puts a cairo context in a permanent error state */
  if (w == 0 || h == 0)
    return;
  NQCanvasImpl_moveTo(thiz, x + w, y + h / 2);
  NQCanvasContext_save(toCanvasContext(thiz));
  NQCanvasContext_translate(toCanvasContext(thiz), x + w / 2, y + h / 2);
  NQCanvasContext_scale(toCanvasContext(thiz), w / 2, h / 2);
  NQCanvasImpl_arc(thiz, 0, 0, 1, 0, 2 * M_PI);
  NQCanvasContext_restore(toCanvasContext(thiz));
  NQCanvasImpl_closePath(thiz);
#endif
}

static inline void NQCanvasImpl_clip(NQCanvas* thiz)
{
#ifdef NQCanvasContext_clipPath
  NQCanvasContext_clipPath(toCanvasContext(thiz), toCanvasPath(thiz));
#elif defined(NQCanvasContext_clip)
  NQCanvasContext_clip(toCanvasContext(thiz));
#else
  NQCanvasImpl_beginPath(thiz);
#endif
}

static inline void NQCanvasImpl_clipRect(NQCanvas* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
#if defined(NQCanvasContext_clipRect)
  NQCanvasContext_clipRect(toCanvasContext(thiz), x, y, w, h);
#else
  NQCanvasImpl_beginPath(thiz);
  NQCanvasImpl_rect(thiz, x, y, w, h);
  NQCanvasImpl_clip(thiz);
#endif
}

const char* NQCanvas_backend(void)
{
  return NQ_CANVAS_BACKEND_NAME;
}

void NQCanvas_save(NQCanvas* thiz)
{
  NQCanvasContext_save(toCanvasContext(thiz));
}

void NQCanvas_restore(NQCanvas* thiz)
{
  NQCanvasContext_restore(toCanvasContext(thiz));
}

void NQCanvas_clipRect(NQCanvas* thiz, double x, double y, double w, double h)
{
  NQCanvasImpl_clipRect(thiz, toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
}

void NQCanvas_clip(NQCanvas* thiz)
{
  NQCanvasImpl_clip(thiz);
}

void NQCanvas_translate(NQCanvas* thiz, double x, double y)
{
  NQCanvasContext_translate(toCanvasContext(thiz), toCanvasFloat(x), toCanvasFloat(y));
}

void NQCanvas_scale(NQCanvas* thiz, double sx, double sy)
{
  NQCanvasContext_scale(toCanvasContext(thiz), toCanvasFloat(sx), toCanvasFloat(sy));
}

void NQCanvas_setColor(NQCanvas* thiz, double r, double g, double b, double a)
{
  NQCanvasImpl_setColor(thiz, toCanvasFloat(r), toCanvasFloat(g), toCanvasFloat(b), toCanvasFloat(a));
}

void NQCanvas_setLineCap(NQCanvas* thiz, NQLineCap lineCap)
{
  NQCanvasImpl_setLineCap(thiz, toCanvasLineCap(lineCap));
}

void NQCanvas_setLineJoin(NQCanvas* thiz, NQLineJoin lineJoin)
{
  NQCanvasPaint_setLineJoin(toCanvasPaint(thiz), toBackenLineJoin(lineJoin));
}

void NQCanvas_setLineWidth(NQCanvas* thiz, double lineWidth)
{
  NQCanvasImpl_setLineWidth(thiz, toCanvasFloat(lineWidth));
}

void NQCanvas_setMiterLimit(NQCanvas* thiz, double miterLimit)
{
  NQCanvasPaint_setMiterLimit(toCanvasPaint(thiz), toCanvasFloat(miterLimit));
}

void NQCanvas_beginPath(NQCanvas* thiz)
{
  NQCanvasImpl_beginPath(thiz);
}

void NQCanvas_moveTo(NQCanvas* thiz, double x, double y)
{
  NQCanvasImpl_moveTo(thiz, toCanvasFloat(x), toCanvasFloat(y));
}

void NQCanvas_lineTo(NQCanvas* thiz, double x, double y)
{
  NQCanvasImpl_lineTo(thiz, toCanvasFloat(x), toCanvasFloat(y));
}

void NQCanvas_arc(NQCanvas* thiz, double x, double y, double radius, double startAngle, double endAngle)
{
  NQCanvasImpl_arc(thiz, toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(radius), toCanvasFloat(startAngle), toCanvasFloat(endAngle));
}

void NQCanvas_rect(NQCanvas* thiz, double x, double y, double width, double height)
{
  NQCanvasImpl_rect(thiz, toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(width), toCanvasFloat(height));
}

void NQCanvas_roundedRect(NQCanvas* thiz, double x, double y, double w, double h, double r)
{
  NQCanvasImpl_roundedRect(thiz, toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h), toCanvasFloat(r));
}

void NQCanvas_ellipse(NQCanvas* thiz, double x, double y, double w, double h)
{
  NQCanvasImpl_ellipse(thiz, toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
}

void NQCanvas_closePath(NQCanvas* thiz)
{
  NQCanvasImpl_closePath(thiz);
}

void NQCanvas_stroke(NQCanvas* thiz)
{
#ifdef NQCanvasContext_drawPath
  NQCanvasPaint_stroke(toCanvasPaint(thiz));
  NQCanvasContext_drawPath(toCanvasContext(thiz), toCanvasPath(thiz), toCanvasPaint(thiz));
#else
  NQCanvasContext_stroke(toCanvasContext(thiz));
#endif
}

void NQCanvas_fill(NQCanvas* thiz)
{
#ifdef NQCanvasContext_drawPath
  NQCanvasPaint_fill(toCanvasPaint(thiz));
  NQCanvasContext_drawPath(toCanvasContext(thiz), toCanvasPath(thiz), toCanvasPaint(thiz));
#else
  NQCanvasContext_fill(toCanvasContext(thiz));
#endif
}

void NQCanvas_fillRect(NQCanvas* thiz, double x, double y, double w, double h)
{
#if defined(NQCanvasContext_drawRect)
  NQCanvasPaint_fill(toCanvasPaint(thiz));
  NQCanvasContext_drawRect(toCanvasContext(thiz), toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h), toCanvasPaint(thiz));
#elif defined(NQCanvasContext_fillRect)
  NQCanvasContext_fillRect(toCanvasContext(thiz), toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
#else
  NQCanvasImpl_beginPath(thiz);
  NQCanvasImpl_rect(thiz, toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
  NQCanvasContext_fill(toCanvasContext(thiz));
#endif
}

void NQCanvas_strokeRect(NQCanvas* thiz, double x, double y, double w, double h)
{
#if defined(NQCanvasContext_drawRect)
  NQCanvasPaint_stroke(toCanvasPaint(thiz));
  NQCanvasContext_drawRect(toCanvasContext(thiz), toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h), toCanvasPaint(thiz));
#elif defined(NQCanvasContext_strokeRect)
  NQCanvasContext_strokeRect(toCanvasContext(thiz), toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
#else
  NQCanvasImpl_beginPath(thiz);
  NQCanvasImpl_rect(thiz, toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
  NQCanvasContext_stroke(toCanvasContext(thiz));
#endif
}

void NQCanvas_fillRoundedRect(NQCanvas* thiz, double x, double y, double w, double h, double r)
{
#ifdef NQCanvasContext_drawRoundRect
  NQCanvasPaint_fill(toCanvasPaint(thiz));
  NQCanvasContext_drawRoundRect(toCanvasContext(thiz), toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h), toCanvasFloat(r), toCanvasPaint(thiz));
#else
  NQCanvasImpl_beginPath(thiz);
  NQCanvasImpl_roundedRect(thiz, toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h), toCanvasFloat(r));
  NQCanvasContext_fill(toCanvasContext(thiz));
#endif
}

void NQCanvas_strokeRoundedRect(NQCanvas* thiz, double x, double y, double w, double h, double r)
{
#ifdef NQCanvasContext_drawRoundRect
  NQCanvasPaint_stroke(toCanvasPaint(thiz));
  NQCanvasContext_drawRoundRect(toCanvasContext(thiz), toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h), toCanvasFloat(r), toCanvasPaint(thiz));
#else
  NQCanvasImpl_beginPath(thiz);
  NQCanvasImpl_roundedRect(thiz, toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h), toCanvasFloat(r));
  NQCanvasContext_stroke(toCanvasContext(thiz));
#endif
}

void NQCanvas_fillEllipse(NQCanvas* thiz, double x, double y, double w, double h)
{
#ifdef NQCanvasContext_fillEllipse
  NQCanvasContext_fillEllipse(toCanvasContext(thiz), toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
#elif defined(NQCanvasContext_drawEllipse)
  NQCanvasPaint_fill(toCanvasPaint(thiz));
  NQCanvasContext_drawEllipse(toCanvasContext(thiz), toCanvasPaint(thiz), toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
#else
  NQCanvasImpl_beginPath(thiz);
  NQCanvasImpl_ellipse(thiz, toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
  NQCanvasContext_fill(toCanvasContext(thiz));
#endif
}

void NQCanvas_strokeEllipse(NQCanvas* thiz, double x, double y, double w, double h)
{
#ifdef NQCanvasContext_strokeEllipse
  NQCanvasContext_strokeEllipse(toCanvasContext(thiz), toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
#elif defined(NQCanvasContext_drawEllipse)
  NQCanvasPaint_stroke(toCanvasPaint(thiz));
  NQCanvasContext_drawEllipse(toCanvasContext(thiz), toCanvasPaint(thiz), toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
#else
  NQCanvasImpl_beginPath(thiz);
  NQCanvasImpl_ellipse(thiz, toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
  NQCanvasContext_stroke(toCanvasContext(thiz));
#endif
}

void NQCanvas_strokeLine(NQCanvas* thiz, double x0, double y0, double x1, double y1)
{
  NQCanvas_beginPath(thiz);
  NQCanvas_moveTo(thiz, x0, y0);
  NQCanvas_lineTo(thiz, x1, y1);
  NQCanvas_stroke(thiz);
}

static inline void polylinePath(NQCanvas* thiz, const double* xy, int n)
{
  NQCanvas_beginPath(thiz);
  if (n > 0)
    NQCanvas_moveTo(thiz, xy[0], xy[1]);
  for (size_t i = 1; i < n; i++)
    NQCanvas_lineTo(thiz, xy[2 * i], xy[2 * i + 1]);
}

void NQCanvas_strokePolyline(NQCanvas* thiz, const double* xy, size_t n)
{
  polylinePath(thiz, xy, n);
  NQCanvas_stroke(thiz);
}

static inline void polygonPath(NQCanvas* thiz, const double* xy, int n)
{
  NQCanvas_beginPath(thiz);
  if (n > 0)
    NQCanvas_moveTo(thiz, xy[0], xy[1]);
  for (size_t i = 1; i < n; i++)
    NQCanvas_lineTo(thiz, xy[2 * i], xy[2 * i + 1]);
  NQCanvas_closePath(thiz);
}

void NQCanvas_fillPolygon(NQCanvas* thiz, const double* xy, size_t n)
{
  polygonPath(thiz, xy, n);
  NQCanvas_fill(thiz);
}

void NQCanvas_strokePolygon(NQCanvas* thiz, const double* xy, size_t n)
{
  polygonPath(thiz, xy, n);
  NQCanvas_stroke(thiz);
}

#endif
