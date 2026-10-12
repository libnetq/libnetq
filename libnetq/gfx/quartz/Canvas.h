/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_GFX_QUARTZ_CANVAS_H
#define _LIBNETQ_GFX_QUARTZ_CANVAS_H

#ifdef CANVAS_BACKEND_QUARTZ

#include <libnetq/Assert.h>

#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_CANVAS_BACKEND_NAME "quartz"

typedef struct CGContext NQCanvasContext;
typedef NQCanvasContext NQCanvasPaint;
typedef NQCanvasContext NQCanvasPath;
typedef CGLineCap NQCanvasLineCap;
typedef double NQCanvasFloat;
typedef struct __CTFontDescriptor NQGFont;

struct NQCanvas {
  CGContextRef context;
  const NQGFont* fontDesc;
  CTFontRef font;
  CGFloat fontSize;
};

#define toCanvasFloat(value) (value)
#define toCanvasContext(canvas) (canvas)->context
#define toCanvasPaint(canvas) (canvas)->context
#define toCanvasPath(canvas) (canvas)->context

static inline void NQCanvasContext_release(NQCanvasContext* thiz)
{
  CGContextRelease(thiz);
}

static inline void NQCanvasContext_save(NQCanvasContext* thiz)
{
  CGContextSaveGState(thiz);
}

static inline void NQCanvasContext_restore(NQCanvasContext* thiz)
{
  CGContextRestoreGState(thiz);
}

static inline void NQCanvasContext_clipRect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  CGContextClipToRect(thiz, CGRectMake(x, y, w, h));
}
#define NQCanvasContext_clipRect NQCanvasContext_clipRect

static inline void NQCanvasContext_clip(NQCanvasContext* thiz)
{
  CGContextClip(thiz);
}
#define NQCanvasContext_clip NQCanvasContext_clip

static inline void NQCanvasContext_translate(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  CGContextTranslateCTM(thiz, x, y);
}

static inline void NQCanvasContext_scale(NQCanvasContext* thiz, NQCanvasFloat sx, NQCanvasFloat sy)
{
  CGContextScaleCTM(thiz, sx, sy);
}

static inline void NQCanvasContext_setColor(NQCanvasContext* thiz, NQCanvasFloat r, NQCanvasFloat g, NQCanvasFloat b, NQCanvasFloat a)
{
  CGContextSetRGBFillColor(thiz, r, g, b, a);
  CGContextSetRGBStrokeColor(thiz, r, g, b, a);
}
#define NQCanvasContext_setColor NQCanvasContext_setColor

static inline void NQCanvasContext_setLineWidth(NQCanvasContext* thiz, NQCanvasFloat v)
{
  CGContextSetLineWidth(thiz, v);
}
#define NQCanvasContext_setLineWidth NQCanvasContext_setLineWidth

static inline NQCanvasLineCap toCanvasLineCap(NQLineCap lineCap)
{
  switch (lineCap) {
  case NQ_LINECAP_BUTT:
    return kCGLineCapButt;
  case NQ_LINECAP_ROUND:
    return kCGLineCapRound;
  case NQ_LINECAP_SQUARE:
    return kCGLineCapSquare;
  }
  NQ_ASSERT_NOT_REACHED();
  return kCGLineCapButt;
}

static inline void NQCanvasContext_setLineCap(NQCanvasContext* thiz, NQCanvasLineCap lineCap)
{
  CGContextSetLineCap(thiz, lineCap);
}
#define NQCanvasContext_setLineCap NQCanvasContext_setLineCap

static inline CGLineJoin toBackenLineJoin(NQLineJoin lineJoin)
{
  switch (lineJoin) {
  case NQ_LINEJOIN_MITER:
    return kCGLineJoinMiter;
  case NQ_LINEJOIN_ROUND:
    return kCGLineJoinRound;
  case NQ_LINEJOIN_BEVEL:
    return kCGLineJoinBevel;
  }
  NQ_ASSERT_NOT_REACHED();
  return kCGLineJoinMiter;
}

static inline void NQCanvasPaint_setLineJoin(NQCanvasPaint* thiz, CGLineJoin lineJoin)
{
  CGContextSetLineJoin(thiz, lineJoin);
}

static inline void NQCanvasPaint_setMiterLimit(NQCanvasPaint* thiz, NQCanvasFloat miterLimit)
{
  CGContextSetMiterLimit(thiz, miterLimit);
}

static inline void NQCanvasContext_beginPath(NQCanvasContext* thiz)
{
  CGContextBeginPath(thiz);
}
#define NQCanvasContext_beginPath NQCanvasContext_beginPath

static inline void NQCanvasContext_moveTo(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  CGContextMoveToPoint(thiz, x, y);
}
#define NQCanvasContext_moveTo NQCanvasContext_moveTo

static inline void NQCanvasContext_lineTo(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  CGContextAddLineToPoint(thiz, x, y);
}
#define NQCanvasContext_lineTo NQCanvasContext_lineTo

static inline void NQCanvasContext_arc(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat radius, NQCanvasFloat startAngle, NQCanvasFloat endAngle)
{
  CGContextAddArc(thiz, x, y, radius, startAngle, endAngle, 0);
}
#define NQCanvasContext_arc NQCanvasContext_arc

static inline void NQCanvasContext_rect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  CGContextAddRect(thiz, CGRectMake(x, y, w, h));
}
#define NQCanvasContext_rect NQCanvasContext_rect

static inline void NQCanvasPath_ellipse(NQCanvasPath* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  CGContextAddEllipseInRect(thiz, CGRectMake(x, y, w, h));
}
#define NQCanvasPath_ellipse NQCanvasPath_ellipse

static inline void NQCanvasContext_fillEllipse(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  CGContextFillEllipseInRect(thiz, CGRectMake(x, y, w, h));
}
#define NQCanvasContext_fillEllipse NQCanvasContext_fillEllipse

static inline void NQCanvasContext_strokeEllipse(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  CGContextStrokeEllipseInRect(thiz, CGRectMake(x, y, w, h));
}
#define NQCanvasContext_strokeEllipse NQCanvasContext_strokeEllipse

static inline void NQCanvasContext_closePath(NQCanvasContext* thiz)
{
  CGContextClosePath(thiz);
}
#define NQCanvasContext_closePath NQCanvasContext_closePath

static inline void NQCanvasContext_stroke(NQCanvasContext* thiz)
{
  CGContextStrokePath(thiz);
}

static inline void NQCanvasContext_fill(NQCanvasContext* thiz)
{
  CGContextFillPath(thiz);
}

static inline void NQCanvasContext_fillRect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  CGContextFillRect(thiz, CGRectMake(x, y, w, h));
}
#define NQCanvasContext_fillRect NQCanvasContext_fillRect

static inline void NQCanvasContext_strokeRect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  CGContextStrokeRect(thiz, CGRectMake(x, y, w, h));
}
#define NQCanvasContext_strokeRect NQCanvasContext_strokeRect

#ifdef __cplusplus
}
#endif

#endif /* CANVAS_BACKEND_QUARTZ */
#endif /* _LIBNETQ_GFX_QUARTZ_CANVAS_H */
