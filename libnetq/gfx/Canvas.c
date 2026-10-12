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

#ifdef CANVAS_BACKEND_BLEND2D
#include "libnetq/gfx/blend2d/Canvas.h"
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
#include <libnetq/string/String.h>

#ifndef NQCanvas_saveImpl
static inline void NQCanvas_saveImpl(NQCanvas* thiz)
{
#ifdef NQCanvasContext_save
  NQCanvasContext_save(getCanvasContext(thiz));
#endif
}
#endif

#ifndef NQCanvas_restoreImpl
static inline void NQCanvas_restoreImpl(NQCanvas* thiz)
{
#ifdef NQCanvasContext_restore
  NQCanvasContext_restore(getCanvasContext(thiz));
#endif
}
#endif

#ifndef NQCanvas_translateImpl
static inline void NQCanvas_translateImpl(NQCanvas* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
#ifdef NQCanvasContext_translate
  NQCanvasContext_translate(getCanvasContext(thiz), x, y);
#endif
}
#endif

#ifndef NQCanvas_scaleImpl
static inline void NQCanvas_scaleImpl(NQCanvas* thiz, NQCanvasFloat sx, NQCanvasFloat sy)
{
#ifdef NQCanvasContext_scale
  NQCanvasContext_scale(getCanvasContext(thiz), sx, sy);
#endif
}
#endif

#ifndef NQCanvas_rotateImpl
static inline void NQCanvas_rotateImpl(NQCanvas* thiz, NQCanvasFloat angle)
{
#ifdef NQCanvasContext_rotate
  NQCanvasContext_rotate(getCanvasContext(thiz), angle);
#endif
}
#endif

#ifndef NQCanvas_setColorImpl
static inline void NQCanvas_setColorImpl(NQCanvas* thiz, NQCanvasFloat r, NQCanvasFloat g, NQCanvasFloat b, NQCanvasFloat a)
{
#if defined(NQCanvasContext_setColor)
  NQCanvasContext_setColor(getCanvasContext(thiz), r, g, b, a);
#elif defined(NQCanvasPaint_setColor)
  NQCanvasPaint_setColor(getCanvasPaint(thiz), r, g, b, a);
#endif
}
#endif

#ifndef NQCanvasl_setLineCapImpl
static inline void NQCanvasl_setLineCapImpl(NQCanvas* thiz, NQCanvasLineCap lineCap)
{
#if defined(NQCanvasContext_setLineCap)
  NQCanvasContext_setLineCap(getCanvasContext(thiz), lineCap);
#elif defined(NQCanvasPaint_setLineCap)
  NQCanvasPaint_setLineCap(getCanvasPaint(thiz), lineCap);
#endif
}
#endif

#ifndef NQCanvas_setLineJoinImpl
static inline void NQCanvas_setLineJoinImpl(NQCanvas* thiz, NQCanvasLineJoin lineJoin)
{
#if defined(NQCanvasContext_setLineJoin)
  NQCanvasContext_setLineJoin(getCanvasContext(thiz), lineJoin);
#elif defined(NQCanvasPaint_setLineJoin)
  NQCanvasPaint_setLineJoin(getCanvasPaint(thiz), lineJoin);
#endif
}
#endif

#ifndef NQCanvas_setMiterLimitImpl
static inline void NQCanvas_setMiterLimitImpl(NQCanvas* thiz, NQCanvasFloat miterLimit)
{
#if defined(NQCanvasContext_setMiterLimit)
  NQCanvasContext_setMiterLimit(getCanvasContext(thiz), miterLimit);
#elif defined(NQCanvasPaint_setMiterLimit)
  NQCanvasPaint_setMiterLimit(getCanvasPaint(thiz), miterLimit);
#endif
}
#endif

#ifndef NQCanvas_setLineWidthImpl
static inline void NQCanvas_setLineWidthImpl(NQCanvas* thiz, NQCanvasFloat lineWidth)
{
#if defined(NQCanvasContext_setLineWidth)
  NQCanvasContext_setLineWidth(getCanvasContext(thiz), lineWidth);
#elif defined(NQCanvasPaint_setLineWidth)
  NQCanvasPaint_setLineWidth(getCanvasPaint(thiz), lineWidth);
#endif
}
#endif

static inline void NQCanvasImpl_beginPath(NQCanvas* thiz)
{
#if defined(NQCanvasContext_beginPath)
  NQCanvasContext_beginPath(getCanvasContext(thiz));
#elif defined(NQCanvasPath_beginPath)
  NQCanvasPath_beginPath(getCanvasPath(thiz));
#else
# error "Canvas backend must define NQCanvasContext_beginPath or NQCanvasPath_beginPath"
#endif
}

static inline void NQCanvasImpl_moveTo(NQCanvas* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
#if defined(NQCanvasContext_moveTo)
  NQCanvasContext_moveTo(getCanvasContext(thiz), x, y);
#elif defined(NQCanvasPath_moveTo)
  NQCanvasPath_moveTo(getCanvasPath(thiz), x, y);
#else
# error "Canvas backend must define NQCanvasContext_moveTo or NQCanvasPath_moveTo"
#endif
}

static inline void NQCanvasImpl_lineTo(NQCanvas* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
#if defined(NQCanvasContext_lineTo)
  NQCanvasContext_lineTo(getCanvasContext(thiz), x, y);
#elif defined(NQCanvasPath_lineTo)
  NQCanvasPath_lineTo(getCanvasPath(thiz), x, y);
#else
# error "Canvas backend must define NQCanvasContext_lineTo or NQCanvasPath_lineTo"
#endif
}

static inline void NQCanvasImpl_arc(NQCanvas* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat radius, NQCanvasFloat startAngle, NQCanvasFloat endAngle)
{
#if defined(NQCanvasContext_arc)
  NQCanvasContext_arc(getCanvasContext(thiz), x, y, radius, startAngle, endAngle);
#elif defined(NQCanvasPath_arc)
  NQCanvasPath_arc(getCanvasPath(thiz), x, y, radius, startAngle, endAngle);
#else
# error "Canvas backend must define NQCanvasContext_arc or NQCanvasPath_arc"
#endif
}

#if defined(NQCanvasContext_currentPoint)
# define NQCanvasImpl_currentPoint(thiz, x, y) NQCanvasContext_currentPoint(getCanvasContext(thiz), x, y)
#elif defined(NQCanvasPath_currentPoint)
# define NQCanvasImpl_currentPoint(thiz, x, y) NQCanvasPath_currentPoint(getCanvasPath(thiz), x, y)
#endif

#if defined(NQCanvasContext_arcNegative)
# define NQCanvasImpl_arcNegative(thiz, x, y, r, a0, a1) NQCanvasContext_arcNegative(getCanvasContext(thiz), x, y, r, a0, a1)
#elif defined(NQCanvasPath_arcNegative)
# define NQCanvasImpl_arcNegative(thiz, x, y, r, a0, a1) NQCanvasPath_arcNegative(getCanvasPath(thiz), x, y, r, a0, a1)
#endif

static inline void NQCanvasImpl_arcTo(NQCanvas* thiz, NQCanvasFloat x1, NQCanvasFloat y1, NQCanvasFloat x2, NQCanvasFloat y2, NQCanvasFloat radius)
{
#if defined(NQCanvasContext_arcTo)
  NQCanvasContext_arcTo(getCanvasContext(thiz), x1, y1, x2, y2, radius);
#elif defined(NQCanvasPath_arcTo)
  NQCanvasPath_arcTo(getCanvasPath(thiz), x1, y1, x2, y2, radius);
#elif defined(NQCanvasImpl_currentPoint) && defined(NQCanvasImpl_arcNegative)
  NQCanvasFloat x0, y0;
  if (!NQCanvasImpl_currentPoint(thiz, &x0, &y0)) {
    NQCanvasImpl_moveTo(thiz, x1, y1);
    x0 = x1;
    y0 = y1;
  }

  const double dx0 = x0 - x1;
  const double dy0 = y0 - y1;
  const double dx2 = x2 - x1;
  const double dy2 = y2 - y1;
  const double cross = dx0 * dy2 - dy0 * dx2;
  /* coincident or collinear points leave no corner to round */
  if (radius == 0 || cross == 0) {
    NQCanvasImpl_lineTo(thiz, x1, y1);
    return;
  }

  const double l0 = hypot(dx0, dy0);
  const double l2 = hypot(dx2, dy2);
  /* r / tan(half the angle at the corner): how far the tangent points sit
   * from (x1, y1) along each line */
  const double d = radius * (l0 * l2 + dx0 * dx2 + dy0 * dy2) / fabs(cross);
  const double t0x = x1 + dx0 / l0 * d;
  const double t0y = y1 + dy0 / l0 * d;
  const double t2x = x1 + dx2 / l2 * d;
  const double t2y = y1 + dy2 / l2 * d;
  /* the centre is a radius off the first line, on the side of (x2, y2) */
  const double side = cross > 0 ? radius : -radius;
  const double cx = t0x - dy0 / l0 * side;
  const double cy = t0y + dx0 / l0 * side;
  const double a0 = atan2(t0y - cy, t0x - cx);
  const double a2 = atan2(t2y - cy, t2x - cx);
  /* the arc's lead-in line runs from the current point to the first tangent point */
  if (cross < 0)
    NQCanvasImpl_arc(thiz, cx, cy, radius, a0, a2);
  else
    NQCanvasImpl_arcNegative(thiz, cx, cy, radius, a0, a2);
#else
# error "Canvas backend must define NQCanvasContext_arcTo, NQCanvasPath_arcTo, or currentPoint and arcNegative for its context or path"
#endif
}

static inline void NQCanvasImpl_closePath(NQCanvas* thiz)
{
#if defined(NQCanvasContext_closePath)
  NQCanvasContext_closePath(getCanvasContext(thiz));
#elif defined(NQCanvasPath_closePath)
  NQCanvasPath_closePath(getCanvasPath(thiz));
#else
# error "Canvas backend must define NQCanvasContext_closePath or NQCanvasPath_closePath"
#endif
}

static inline void NQCanvasImpl_rect(NQCanvas* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
#if defined(NQCanvasContext_rect)
  NQCanvasContext_rect(getCanvasContext(thiz), x, y, w, h);
#elif defined(NQCanvasPath_rect)
  NQCanvasPath_rect(getCanvasPath(thiz), x, y, w, h);
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
  NQCanvasContext_roundedRect(getCanvasContext(thiz), x, y, w, h, r);
#elif defined(NQCanvasPath_roundedRect)
  NQCanvasPath_roundedRect(getCanvasPath(thiz), x, y, w, h, r);
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
#if defined(NQCanvasContext_ellipse)
  NQCanvasContext_ellipse(getCanvasContext(thiz), x, y, w, h);
#elif defined(NQCanvasPath_ellipse)
  NQCanvasPath_ellipse(getCanvasPath(thiz), x, y, w, h);
#else
  /* a zero scale is a singular matrix, which puts a cairo context in a permanent error state */
  if (w == 0 || h == 0)
    return;
  NQCanvasImpl_moveTo(thiz, x + w, y + h / 2);
  NQCanvas_saveImpl(thiz);
  NQCanvas_translateImpl(thiz, x + w / 2, y + h / 2);
  NQCanvas_scaleImpl(thiz, w / 2, h / 2);
  NQCanvasImpl_arc(thiz, 0, 0, 1, 0, 2 * M_PI);
  NQCanvas_restoreImpl(thiz);
  NQCanvasImpl_closePath(thiz);
#endif
}

static inline void NQCanvasImpl_clip(NQCanvas* thiz)
{
#ifdef NQCanvasContext_clipPath
  NQCanvasContext_clipPath(getCanvasContext(thiz), getCanvasPath(thiz));
#elif defined(NQCanvasContext_clip)
  NQCanvasContext_clip(getCanvasContext(thiz));
#else
  NQCanvasImpl_beginPath(thiz);
#endif
}

#ifndef NQCanvas_clipRectImpl
static inline void NQCanvas_clipRectImpl(NQCanvas* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
#if defined(NQCanvasContext_clipRect)
  NQCanvasContext_clipRect(getCanvasContext(thiz), x, y, w, h);
#else
  NQCanvasImpl_beginPath(thiz);
  NQCanvasImpl_rect(thiz, x, y, w, h);
  NQCanvasImpl_clip(thiz);
#endif
}
#endif

static inline void NQCanvas_strokeImpl(NQCanvas* thiz)
{
#ifdef NQCanvasContext_drawPath
  NQCanvasPaint_stroke(getCanvasPaint(thiz));
  NQCanvasContext_drawPath(getCanvasContext(thiz), getCanvasPath(thiz), getCanvasPaint(thiz));
#elif defined(NQCanvasContext_strokePath)
  NQCanvasContext_strokePath(getCanvasContext(thiz), getCanvasPath(thiz));
#else
  NQCanvasContext_stroke(getCanvasContext(thiz));
#endif
}

static inline void NQCanvas_fillImpl(NQCanvas* thiz)
{
#ifdef NQCanvasContext_drawPath
  NQCanvasPaint_fill(getCanvasPaint(thiz));
  NQCanvasContext_drawPath(getCanvasContext(thiz), getCanvasPath(thiz), getCanvasPaint(thiz));
#elif defined(NQCanvasContext_fillPath)
  NQCanvasContext_fillPath(getCanvasContext(thiz), getCanvasPath(thiz));
#else
  NQCanvasContext_fill(getCanvasContext(thiz));
#endif
}

#ifndef NQCanvas_fontFaceRelease
static inline void NQCanvas_fontFaceReleaseImpl(NQCanvas* thiz, NQCanvasFontFace* fontFace)
{
#if defined(NQCanvasFontFace_release)
  NQ_UNUSED_PARAM(thiz);
  NQCanvasFontFace_release(fontFace);
#else
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(fontFace);
#endif
}
#endif

#ifndef NQCanvas_setFontFaceImpl
static inline void NQCanvas_setFontFaceImpl(NQCanvas* thiz, NQCanvasFontFace* fontFace)
{
#if defined(NQCanvasContext_setFontFace)
  NQCanvasContext_setFontFace(getCanvasContext(thiz), fontFace);
#elif defined(NQCanvasFont_setFace)
  NQCanvasFont_setFace(getCanvasFont(thiz), fontFace);
#else
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(fontFace);
#endif
}
#endif

#ifndef NQCanvas_setFontSizeImpl
static inline void NQCanvas_setFontSizeImpl(NQCanvas* thiz, NQCanvasFloat fontSize)
{
#if defined(NQCanvasContext_setFontSize)
  NQCanvasContext_setFontSize(getCanvasContext(thiz), fontSize);
#elif defined(NQCanvasFont_setSize)
  NQCanvasFont_setSize(getCanvasFont(thiz), fontSize);
#else
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(fontSize);
#endif
}
#endif

#ifndef NQCanvas_fontMetricsImpl
static inline void NQCanvas_fontMetricsImpl(NQCanvas* thiz, NQCanvasFontMetrics* fontMetrics)
{
#if defined(NQCanvasContext_fontMetrics)
  NQCanvasContext_fontMetrics(getCanvasContext(thiz), fontMetrics);
#elif defined(NQCanvasFont_metrics)
  NQCanvasFont_metrics(getCanvasFont(thiz), fontMetrics);
#else
  NQ_UNUSED_PARAM(thiz);
#endif
}
#endif

#ifndef NQCanvas_textWidthImpl
static inline NQCanvasFloat NQCanvas_textWidthImpl(NQCanvas* thiz, const char* text, size_t length)
{
  if (length == 0)
    return 0;
#if defined(NQCanvasContext_textWidth)
  return NQCanvasContext_textWidth(getCanvasContext(thiz), text, length);
#elif defined(NQCanvasFont_textWidth)
  return NQCanvasFont_textWidth(getCanvasFont(thiz), text, length);
#else
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(text);
  NQ_UNUSED_PARAM(length);
  return 0;
#endif
}
#endif

#ifdef getCanvasTextState
/* Moves the anchor (x, y) to the left end of the alphabetic baseline, where
 * the backends start drawing */
static void textOrigin(NQCanvas* thiz, const char* text, size_t length, NQCanvasFloat* x, NQCanvasFloat* y)
{
  const NQTextState* state = getCanvasTextState(thiz);
  const int rtl = state->direction == NQ_TEXT_DIRECTION_RTL;

  switch (state->align) {
  case NQ_TEXT_ALIGN_START:
    if (rtl)
      *x -= NQCanvas_textWidthImpl(thiz, text, length);
    break;
  case NQ_TEXT_ALIGN_END:
    if (!rtl)
      *x -= NQCanvas_textWidthImpl(thiz, text, length);
    break;
  case NQ_TEXT_ALIGN_LEFT:
    break;
  case NQ_TEXT_ALIGN_RIGHT:
    *x -= NQCanvas_textWidthImpl(thiz, text, length);
    break;
  case NQ_TEXT_ALIGN_CENTER:
    *x -= NQCanvas_textWidthImpl(thiz, text, length) / 2;
    break;
  }

  if (state->baseline == NQ_TEXT_BASELINE_ALPHABETIC)
    return;

  /* The em box is taken to span the ascent and descent; none of the backends
   * report hanging or ideographic baselines, so those are approximated */
  NQCanvasFontMetrics m;
  NQCanvas_fontMetricsImpl(thiz, &m);
  switch (state->baseline) {
  case NQ_TEXT_BASELINE_ALPHABETIC:
    break;
  case NQ_TEXT_BASELINE_TOP:
    *y += NQCanvasFontMetrics_ascent(&m);
    break;
  case NQ_TEXT_BASELINE_HANGING:
    *y += NQCanvasFontMetrics_ascent(&m) * 0.8;
    break;
  case NQ_TEXT_BASELINE_MIDDLE:
    *y += (NQCanvasFontMetrics_ascent(&m) - NQCanvasFontMetrics_descent(&m)) / 2;
    break;
  case NQ_TEXT_BASELINE_IDEOGRAPHIC:
  case NQ_TEXT_BASELINE_BOTTOM:
    *y -= NQCanvasFontMetrics_descent(&m);
    break;
  }
}
#endif

#ifndef NQCanvas_fillImplText
static inline void NQCanvas_fillImplText(NQCanvas* thiz, const char* text, size_t length, NQCanvasFloat x, NQCanvasFloat y)
{
#ifdef getCanvasTextState
  textOrigin(thiz, text, length, &x, &y);
#endif

#if defined(NQCanvasContext_drawSimpleText)
  NQCanvasPaint_fill(getCanvasPaint(thiz));
  NQCanvasContext_drawSimpleText(getCanvasContext(thiz), text, length, x, y, getCanvasFont(thiz), getCanvasPaint(thiz));
#elif defined(NQCanvasFont_fillText)
  NQCanvasFont_fillText(getCanvasContext(thiz), getCanvasFont(thiz), text, length, x, y);
#elif defined(NQCanvasContext_fillText)
  NQCanvasContext_fillText(getCanvasContext(thiz), text, length, x, y);
#elif defined(NQCanvasContext_strokeText)
  NQCanvasContext_strokeText(getCanvasContext(thiz), text, length, x, y);
#else
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(text);
  NQ_UNUSED_PARAM(length);
  NQ_UNUSED_PARAM(x);
  NQ_UNUSED_PARAM(y);
#endif
}
#endif

#ifndef NQCanvas_strokeImplText
static inline void NQCanvas_strokeImplText(NQCanvas* thiz, const char* text, size_t length, NQCanvasFloat x, NQCanvasFloat y)
{
#ifdef getCanvasTextState
  textOrigin(thiz, text, length, &x, &y);
#endif

#if defined(NQCanvasContext_drawSimpleText)
  NQCanvasPaint_stroke(getCanvasPaint(thiz));
  NQCanvasContext_drawSimpleText(getCanvasContext(thiz), text, length, x, y, getCanvasFont(thiz), getCanvasPaint(thiz));
#elif defined(NQCanvasFont_strokeText)
  NQCanvasFont_strokeText(getCanvasContext(thiz), getCanvasFont(thiz), text, length, x, y);
#elif defined(NQCanvasContext_strokeText)
  NQCanvasContext_strokeText(getCanvasContext(thiz), text, length, x, y);
#elif defined(NQCanvasContext_fillText)
  NQCanvasContext_fillText(getCanvasContext(thiz), text, length, x, y);
#else
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(text);
  NQ_UNUSED_PARAM(length);
  NQ_UNUSED_PARAM(x);
  NQ_UNUSED_PARAM(y);
#endif
}
#endif

#ifndef NQCanvas_imageCreateRGBAImpl
static inline NQCanvasImage* NQCanvas_imageCreateRGBAImpl(NQCanvas* thiz, const void* pixels, int width, int height, int stride)
{
  if (!pixels || width <= 0 || height <= 0 || (int64_t)stride < (int64_t)width * 4)
    return NULL;

#if defined(NQCanvasContext_createImageRGBA)
  return NQCanvasContext_createImageRGBA(getCanvasContext(thiz), pixels, width, height, stride);
#elif defined(NQCanvasImage_createRGBA)
  NQ_UNUSED_PARAM(thiz);
  return NQCanvasImage_createRGBA(pixels, width, height, stride);
#else
  NQ_UNUSED_PARAM(thiz);
  return NULL;
#endif
}
#endif

#ifndef NQCanvas_imageReleaseImpl
static inline void NQCanvas_imageReleaseImpl(NQCanvas* thiz, NQCanvasImage* image)
{
#if defined(NQCanvasContext_deleteImage)
  NQCanvasContext_deleteImage(getCanvasContext(thiz), image);
#elif defined(NQCanvasImage_release)
  NQ_UNUSED_PARAM(thiz);
  NQCanvasImage_release(image);
#else
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(image);
#endif
}
#endif

#ifndef NQCanvas_imageWidthImpl
static inline int NQCanvas_imageWidthImpl(NQCanvas* thiz, NQCanvasImage* image)
{
#if defined(NQCanvasImage_width)
  NQ_UNUSED_PARAM(thiz);
  return NQCanvasImage_width(image);
#elif defined(NQCanvasContext_imageSize)
  int w, h;
  NQCanvasContext_imageSize(getCanvasContext(thiz), image, &w, &h);
  return w;
#else
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(image);
  return 0;
#endif
}
#endif

#ifndef NQCanvas_imageHeightImpl
static inline int NQCanvas_imageHeightImpl(NQCanvas* thiz, NQCanvasImage* image)
{
#if defined(NQCanvasImage_height)
  NQ_UNUSED_PARAM(thiz);
  return NQCanvasImage_height(image);
#elif defined(NQCanvasContext_imageSize)
  int w, h;
  NQCanvasContext_imageSize(getCanvasContext(thiz), image, &w, &h);
  return h;
#else
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(image);
  return 0;
#endif
}
#endif

#ifndef NQCanvas_imageSizeImpl
static inline void NQCanvas_imageSizeImpl(NQCanvas* thiz, NQCanvasImage* image, int* w, int* h)
{
#if defined(NQCanvasContext_imageSize)
  NQCanvasContext_imageSize(getCanvasContext(thiz), image, w, h);
#else
  *w = NQCanvas_imageWidthImpl(thiz, image);
  *h = NQCanvas_imageHeightImpl(thiz, image);
#endif
}
#endif

#ifndef NQCanvas_drawImageImpl
static inline void NQCanvas_drawImageImpl(NQCanvas* thiz, NQCanvasImage* image,
  NQCanvasFloat sx, NQCanvasFloat sy, NQCanvasFloat sw, NQCanvasFloat sh,
  NQCanvasFloat dx, NQCanvasFloat dy, NQCanvasFloat dw, NQCanvasFloat dh)
{
#if defined(NQCanvasContext_drawImage)
  NQCanvasContext_drawImage(getCanvasContext(thiz), image, sx, sy, sw, sh, dx, dy, dw, dh);
#endif
}
#endif

const char* NQCanvas_backend(void)
{
  return NQ_CANVAS_BACKEND_NAME;
}

void NQCanvas_save(NQCanvas* thiz)
{
  NQCanvas_saveImpl(thiz);
}

void NQCanvas_restore(NQCanvas* thiz)
{
  NQCanvas_restoreImpl(thiz);
}

void NQCanvas_clipRect(NQCanvas* thiz, double x, double y, double w, double h)
{
  NQCanvas_clipRectImpl(thiz, toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
}

void NQCanvas_clip(NQCanvas* thiz)
{
  NQCanvasImpl_clip(thiz);
}

void NQCanvas_translate(NQCanvas* thiz, double x, double y)
{
  NQCanvas_translateImpl(thiz, toCanvasFloat(x), toCanvasFloat(y));
}

void NQCanvas_scale(NQCanvas* thiz, double sx, double sy)
{
  NQCanvas_scaleImpl(thiz, toCanvasFloat(sx), toCanvasFloat(sy));
}

void NQCanvas_rotate(NQCanvas* thiz, double angle)
{
  NQCanvas_rotateImpl(thiz, toCanvasFloat(angle));
}

void NQCanvas_setColor(NQCanvas* thiz, double r, double g, double b, double a)
{
  NQCanvas_setColorImpl(thiz, toCanvasFloat(r), toCanvasFloat(g), toCanvasFloat(b), toCanvasFloat(a));
}

void NQCanvas_setLineCap(NQCanvas* thiz, NQLineCap lineCap)
{
  NQCanvasl_setLineCapImpl(thiz, toCanvasLineCap(lineCap));
}

void NQCanvas_setLineJoin(NQCanvas* thiz, NQLineJoin lineJoin)
{
  NQCanvas_setLineJoinImpl(thiz, toCanvasLineJoin(lineJoin));
}

void NQCanvas_setLineWidth(NQCanvas* thiz, double lineWidth)
{
  NQCanvas_setLineWidthImpl(thiz, toCanvasFloat(lineWidth));
}

void NQCanvas_setMiterLimit(NQCanvas* thiz, double miterLimit)
{
  NQCanvas_setMiterLimitImpl(thiz, toCanvasFloat(miterLimit));
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

void NQCanvas_arcTo(NQCanvas* thiz, double x1, double y1, double x2, double y2, double radius)
{
  NQCanvasImpl_arcTo(thiz, toCanvasFloat(x1), toCanvasFloat(y1), toCanvasFloat(x2), toCanvasFloat(y2), toCanvasFloat(radius));
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
  NQCanvas_strokeImpl(thiz);
}

void NQCanvas_fill(NQCanvas* thiz)
{
  NQCanvas_fillImpl(thiz);
}

void NQCanvas_fillRect(NQCanvas* thiz, double x, double y, double w, double h)
{
#if defined(NQCanvasContext_drawRect)
  NQCanvasPaint_fill(getCanvasPaint(thiz));
  NQCanvasContext_drawRect(getCanvasContext(thiz), toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h), getCanvasPaint(thiz));
#elif defined(NQCanvasContext_fillRect)
  NQCanvasContext_fillRect(getCanvasContext(thiz), toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
#else
  NQCanvasImpl_beginPath(thiz);
  NQCanvasImpl_rect(thiz, toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
  NQCanvas_fillImpl(thiz);
#endif
}

void NQCanvas_strokeRect(NQCanvas* thiz, double x, double y, double w, double h)
{
#if defined(NQCanvasContext_drawRect)
  NQCanvasPaint_stroke(getCanvasPaint(thiz));
  NQCanvasContext_drawRect(getCanvasContext(thiz), toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h), getCanvasPaint(thiz));
#elif defined(NQCanvasContext_strokeRect)
  NQCanvasContext_strokeRect(getCanvasContext(thiz), toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
#else
  NQCanvasImpl_beginPath(thiz);
  NQCanvasImpl_rect(thiz, toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
  NQCanvas_strokeImpl(thiz);
#endif
}

void NQCanvas_fillRoundedRect(NQCanvas* thiz, double x, double y, double w, double h, double r)
{
#ifdef NQCanvasContext_drawRoundRect
  NQCanvasPaint_fill(getCanvasPaint(thiz));
  NQCanvasContext_drawRoundRect(getCanvasContext(thiz), toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h), toCanvasFloat(r), getCanvasPaint(thiz));
#else
  NQCanvasImpl_beginPath(thiz);
  NQCanvasImpl_roundedRect(thiz, toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h), toCanvasFloat(r));
  NQCanvas_fillImpl(thiz);
#endif
}

void NQCanvas_strokeRoundedRect(NQCanvas* thiz, double x, double y, double w, double h, double r)
{
#ifdef NQCanvasContext_drawRoundRect
  NQCanvasPaint_stroke(getCanvasPaint(thiz));
  NQCanvasContext_drawRoundRect(getCanvasContext(thiz), toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h), toCanvasFloat(r), getCanvasPaint(thiz));
#else
  NQCanvasImpl_beginPath(thiz);
  NQCanvasImpl_roundedRect(thiz, toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h), toCanvasFloat(r));
  NQCanvas_strokeImpl(thiz);
#endif
}

void NQCanvas_fillEllipse(NQCanvas* thiz, double x, double y, double w, double h)
{
#ifdef NQCanvasContext_fillEllipse
  NQCanvasContext_fillEllipse(getCanvasContext(thiz), toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
#elif defined(NQCanvasContext_drawEllipse)
  NQCanvasPaint_fill(getCanvasPaint(thiz));
  NQCanvasContext_drawEllipse(getCanvasContext(thiz), getCanvasPaint(thiz), toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
#else
  NQCanvasImpl_beginPath(thiz);
  NQCanvasImpl_ellipse(thiz, toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
  NQCanvas_fillImpl(thiz);
#endif
}

void NQCanvas_strokeEllipse(NQCanvas* thiz, double x, double y, double w, double h)
{
#ifdef NQCanvasContext_strokeEllipse
  NQCanvasContext_strokeEllipse(getCanvasContext(thiz), toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
#elif defined(NQCanvasContext_drawEllipse)
  NQCanvasPaint_stroke(getCanvasPaint(thiz));
  NQCanvasContext_drawEllipse(getCanvasContext(thiz), getCanvasPaint(thiz), toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
#else
  NQCanvasImpl_beginPath(thiz);
  NQCanvasImpl_ellipse(thiz, toCanvasFloat(x), toCanvasFloat(y), toCanvasFloat(w), toCanvasFloat(h));
  NQCanvas_strokeImpl(thiz);
#endif
}

void NQCanvas_strokeLine(NQCanvas* thiz, double x0, double y0, double x1, double y1)
{
  NQCanvasImpl_beginPath(thiz);
  NQCanvasImpl_moveTo(thiz, x0, y0);
  NQCanvasImpl_lineTo(thiz, x1, y1);
  NQCanvas_strokeImpl(thiz);
}

static inline void polylinePath(NQCanvas* thiz, const double* xy, int n)
{
  NQCanvasImpl_beginPath(thiz);
  if (n > 0)
    NQCanvasImpl_moveTo(thiz, xy[0], xy[1]);
  for (size_t i = 1; i < n; i++)
    NQCanvasImpl_lineTo(thiz, xy[2 * i], xy[2 * i + 1]);
}

void NQCanvas_strokePolyline(NQCanvas* thiz, const double* xy, size_t n)
{
  polylinePath(thiz, xy, n);
  NQCanvas_strokeImpl(thiz);
}

static inline void polygonPath(NQCanvas* thiz, const double* xy, int n)
{
  NQCanvasImpl_beginPath(thiz);
  if (n > 0)
    NQCanvasImpl_moveTo(thiz, xy[0], xy[1]);
  for (size_t i = 1; i < n; i++)
    NQCanvasImpl_lineTo(thiz, xy[2 * i], xy[2 * i + 1]);
  NQCanvasImpl_closePath(thiz);
}

void NQCanvas_fillPolygon(NQCanvas* thiz, const double* xy, size_t n)
{
  polygonPath(thiz, xy, n);
  NQCanvas_fillImpl(thiz);
}

void NQCanvas_strokePolygon(NQCanvas* thiz, const double* xy, size_t n)
{
  polygonPath(thiz, xy, n);
  NQCanvas_strokeImpl(thiz);
}

void NQCanvas_setTextAlign(NQCanvas* thiz, NQTextAlign align)
{
#ifdef getCanvasTextState
  getCanvasTextState(thiz)->align = align;
#else
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(align);
#endif
}

void NQCanvas_setTextBaseline(NQCanvas* thiz, NQTextBaseline baseline)
{
#ifdef getCanvasTextState
  getCanvasTextState(thiz)->baseline = baseline;
#else
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(baseline);
#endif
}

void NQCanvas_setTextDirection(NQCanvas* thiz, NQTextDirection direction)
{
#ifdef getCanvasTextState
  getCanvasTextState(thiz)->direction = direction;
#else
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(direction);
#endif
}

void NQCanvas_fontFaceRelease(NQCanvas* thiz, NQCanvasFontFace* fontFace)
{
  NQCanvas_fontFaceReleaseImpl(thiz, fontFace);
}

void NQCanvas_setFontFace(NQCanvas* thiz, NQCanvasFontFace* fontFace)
{
  NQCanvas_setFontFaceImpl(thiz, fontFace);
}

void NQCanvas_setFontSize(NQCanvas* thiz, double fontSize)
{
  NQCanvas_setFontSizeImpl(thiz, toCanvasFloat(fontSize));
}

void NQCanvas_fontMetrics(NQCanvas* thiz, NQFontMetrics* fontMetrics)
{
  NQCanvasFontMetrics m;
  NQCanvas_fontMetricsImpl(thiz, &m);
  fontMetrics->ascent = NQCanvasFontMetrics_ascent(&m);
  fontMetrics->descent = NQCanvasFontMetrics_descent(&m);
}

double NQCanvas_textWidth(NQCanvas* thiz, const char* text)
{
  return NQCanvas_textWidthImpl(thiz, text, NQStrlen(text));
}

double NQCanvas_textWidth2(NQCanvas* thiz, const char* text, size_t length)
{
  return NQCanvas_textWidthImpl(thiz, text, length);
}

void NQCanvas_strokeText(NQCanvas* thiz, const char* text, double x, double y)
{
  NQCanvas_strokeImplText(thiz, text, NQStrlen(text), toCanvasFloat(x), toCanvasFloat(y));
}

void NQCanvas_strokeText2(NQCanvas* thiz, const char* text, size_t length, double x, double y)
{
  NQCanvas_strokeImplText(thiz, text, length, toCanvasFloat(x), toCanvasFloat(y));
}

void NQCanvas_fillText(NQCanvas* thiz, const char* text, double x, double y)
{
  NQCanvas_fillImplText(thiz, text, NQStrlen(text), toCanvasFloat(x), toCanvasFloat(y));
}

void NQCanvas_fillText2(NQCanvas* thiz, const char* text, size_t length, double x, double y)
{
  NQCanvas_fillImplText(thiz, text, length, toCanvasFloat(x), toCanvasFloat(y));
}

NQCanvasImage* NQCanvas_imageCreateRGBA(NQCanvas* thiz, const void* pixels, int width, int height, int stride)
{
  return NQCanvas_imageCreateRGBAImpl(thiz, pixels, width, height, stride);
}

void NQCanvas_imageRelease(NQCanvas* thiz, NQCanvasImage* image)
{
  NQCanvas_imageReleaseImpl(thiz, image);
}

int NQCanvas_imageWidth(NQCanvas* thiz, NQCanvasImage* image)
{
  return NQCanvas_imageWidthImpl(thiz, image);
}

int NQCanvas_imageHeight(NQCanvas* thiz, NQCanvasImage* image)
{
  return NQCanvas_imageHeightImpl(thiz, image);
}

void NQCanvas_drawImage(NQCanvas* thiz, NQCanvasImage* image, double x, double y)
{
  int w, h;
  NQCanvas_imageSizeImpl(thiz, image, &w, &h);
  NQCanvas_drawImage3(thiz, image, 0, 0, w, h, x, y, w, h);
}

void NQCanvas_drawImage2(NQCanvas* thiz, NQCanvasImage* image, double x, double y, double dw, double dh)
{
  int w, h;
  NQCanvas_imageSizeImpl(thiz, image, &w, &h);
  NQCanvas_drawImage3(thiz, image, 0, 0, w, h, x, y, dw, dh);
}

/* A negative size puts the rect on the other side of its corner */
static inline void normalizeSpan(double* start, double* size)
{
  if (*size < 0) {
    *start += *size;
    *size = -*size;
  }
}

/* Follows the drawImage steps of the HTML canvas spec, so the backends only
 * see positive sizes and a source rect inside the image */
void NQCanvas_drawImage3(NQCanvas* thiz, NQCanvasImage* image, double sx, double sy, double sw, double sh, double dx, double dy, double dw, double dh)
{
  int width, height;
  NQCanvas_imageSizeImpl(thiz, image, &width, &height);
  if (width <= 0 || height <= 0)
    return;

  if (!isfinite(sx) || !isfinite(sy) || !isfinite(sw) || !isfinite(sh) ||
      !isfinite(dx) || !isfinite(dy) || !isfinite(dw) || !isfinite(dh))
    return;

  normalizeSpan(&sx, &sw);
  normalizeSpan(&sy, &sh);
  normalizeSpan(&dx, &dw);
  normalizeSpan(&dy, &dh);
  if (sw == 0 || sh == 0 || dw == 0 || dh == 0)
    return;

  /* cut the source rect to the image and the destination rect in proportion */
  const double ax = dw / sw;
  const double ay = dh / sh;
  if (sx < 0) {
    dx -= sx * ax;
    sw += sx;
    sx = 0;
  }
  if (sy < 0) {
    dy -= sy * ay;
    sh += sy;
    sy = 0;
  }
  if (sx + sw > width)
    sw = width - sx;
  if (sy + sh > height)
    sh = height - sy;
  if (sw <= 0 || sh <= 0)
    return;
  dw = sw * ax;
  dh = sh * ay;

  NQCanvas_drawImageImpl(thiz, image, toCanvasFloat(sx), toCanvasFloat(sy), toCanvasFloat(sw), toCanvasFloat(sh), toCanvasFloat(dx), toCanvasFloat(dy), toCanvasFloat(dw), toCanvasFloat(dh));
}

void NQCanvas_flush(NQCanvas* thiz)
{
#ifdef NQCanvasImpl_flush
  NQCanvasImpl_flush(thiz);
#endif
}

#endif
