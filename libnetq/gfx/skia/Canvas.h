/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_GFX_SKIA_CANVAS_H
#define _LIBNETQ_GFX_SKIA_CANVAS_H

#ifdef CANVAS_BACKEND_SKIA

#include <libnetq/Assert.h>

#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkRRect.h"
#include "include/core/SkRect.h"

#include <memory> // for std::unique_ptr
#include <utility> // for std::move

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_CANVAS_BACKEND_NAME "skia"

typedef struct NQCanvas NQCanvas;
typedef SkCanvas NQCanvasContext;
typedef SkPaint NQCanvasPaint;
typedef SkPaint::Cap NQCanvasLineCap;
typedef SkPathBuilder NQCanvasPath;
typedef SkScalar NQCanvasFloat;

#define toCanvasFloat(value) static_cast<SkScalar>(value)
#define toCanvasContext(canvas) (canvas)->sk.get()
#define toCanvasPaint(canvas) &(canvas)->paint
#define toCanvasPath(canvas) &(canvas)->path

struct NQCanvas {
  std::unique_ptr<NQCanvasContext> sk;
  void* owned_pixels;
  NQCanvasPaint paint;
  NQCanvasPath path;
  SkFont font;
};

static inline void NQCanvasContext_save(NQCanvasContext* thiz)
{
  thiz->save();
}

static inline void NQCanvasContext_restore(NQCanvasContext* thiz)
{
  thiz->restore();
}

static inline void NQCanvasContext_clipRect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  thiz->clipRect(SkRect::MakeXYWH(x, y, w, h), true);
}
#define NQCanvasContext_clipRect NQCanvasContext_clipRect

static inline void NQCanvasContext_clipPath(NQCanvasContext* thiz, NQCanvasPath* path)
{
  thiz->clipPath(path->detach(), true);
}
#define NQCanvasContext_clipPath NQCanvasContext_clipPath

static inline void NQCanvasContext_translate(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  thiz->translate(x, y);
}

static inline void NQCanvasContext_scale(NQCanvasContext* thiz, NQCanvasFloat sx, NQCanvasFloat sy)
{
  thiz->scale(sx, sy);
}

static inline void NQCanvasPaint_setColor(NQCanvasPaint* thiz, NQCanvasFloat r, NQCanvasFloat g, NQCanvasFloat b, NQCanvasFloat a)
{
  thiz->setColor4f(SkColor4f{r, g, b, a});
}
#define NQCanvasPaint_setColor NQCanvasPaint_setColor

static inline void NQCanvasPaint_setLineWidth(NQCanvasPaint* thiz, NQCanvasFloat v)
{
  thiz->setStrokeWidth(v);
}
#define NQCanvasPaint_setLineWidth NQCanvasPaint_setLineWidth

static inline NQCanvasLineCap toCanvasLineCap(NQLineCap lineCap)
{
  switch (lineCap) {
  case NQ_LINECAP_BUTT:
    return SkPaint::kButt_Cap;
  case NQ_LINECAP_ROUND:
    return SkPaint::kRound_Cap;
  case NQ_LINECAP_SQUARE:
    return SkPaint::kSquare_Cap;
  }
  NQ_ASSERT_NOT_REACHED();
  return SkPaint::kButt_Cap;
}

static inline void NQCanvasPaint_setLineCap(NQCanvasPaint* thiz, NQCanvasLineCap lineCap)
{
  thiz->setStrokeCap(lineCap);
}
#define NQCanvasPaint_setLineCap NQCanvasPaint_setLineCap

static inline SkPaint::Join toBackenLineJoin(NQLineJoin lineJoin)
{
  switch (lineJoin) {
  case NQ_LINEJOIN_MITER:
    return SkPaint::kMiter_Join;
  case NQ_LINEJOIN_ROUND:
    return SkPaint::kRound_Join;
  case NQ_LINEJOIN_BEVEL:
    return SkPaint::kBevel_Join;
  }
  NQ_ASSERT_NOT_REACHED();
  return SkPaint::kMiter_Join;
}

static inline void NQCanvasPaint_setLineJoin(NQCanvasPaint* thiz, SkPaint::Join lineJoin)
{
  thiz->setStrokeJoin(lineJoin);
}

static inline void NQCanvasPaint_setMiterLimit(NQCanvasPaint* thiz, NQCanvasFloat miterLimit)
{
  thiz->setStrokeMiter(miterLimit);
}

static inline void NQCanvasPath_beginPath(NQCanvasPath* thiz)
{
  thiz->reset();
}
#define NQCanvasPath_beginPath NQCanvasPath_beginPath

static inline void NQCanvasPath_moveTo(NQCanvasPath* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  thiz->moveTo(x, y);
}
#define NQCanvasPath_moveTo NQCanvasPath_moveTo

static inline void NQCanvasPath_lineTo(NQCanvasPath* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  thiz->lineTo(x, y);
}
#define NQCanvasPath_lineTo NQCanvasPath_lineTo

static inline void NQCanvasPath_arc(NQCanvasPath* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat radius, NQCanvasFloat startAngle, NQCanvasFloat endAngle)
{
  const SkRect oval = SkRect::MakeLTRB(x - radius, y - radius, x + radius, y + radius);
  SkScalar start = SkRadiansToDegrees(startAngle);
  SkScalar sweep = SkRadiansToDegrees(endAngle - startAngle);
  if (sweep >= 360) {
    thiz->arcTo(oval, start, 180, false);
    thiz->arcTo(oval, start + 180, 180, false);
    return;
  }
  while (sweep < 0)
    sweep += 360;
  thiz->arcTo(oval, start, sweep, false);
}
#define NQCanvasPath_arc NQCanvasPath_arc

static inline void NQCanvasPath_rect(NQCanvasPath* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  thiz->addRect(SkRect::MakeXYWH(x, y, w, h));
}
#define NQCanvasPath_rect NQCanvasPath_rect

static inline void NQCanvasPath_roundedRect(NQCanvasPath* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h, NQCanvasFloat r)
{
  thiz->addRRect(SkRRect::MakeRectXY(SkRect::MakeXYWH(x, y, w, h), r, r));
}
#define NQCanvasPath_roundedRect NQCanvasPath_roundedRect

static inline void NQCanvasPath_ellipse(NQCanvasPath* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  thiz->addOval(SkRect::MakeXYWH(x, y, w, h));
}
#define NQCanvasPath_ellipse NQCanvasPath_ellipse

static inline void NQCanvasPath_closePath(NQCanvasPath* thiz)
{
  thiz->close();
}
#define NQCanvasPath_closePath NQCanvasPath_closePath

static inline void NQCanvasPaint_stroke(NQCanvasPaint* thiz)
{
  thiz->setStroke(true);
}
#define NQCanvasPaint_stroke NQCanvasPaint_stroke

static inline void NQCanvasPaint_fill(NQCanvasPaint* thiz)
{
  thiz->setStroke(false);
}
#define NQCanvasPaint_fill NQCanvasPaint_fill

static inline void NQCanvasContext_drawPath(NQCanvasContext* thiz, NQCanvasPath* path, NQCanvasPaint* paint)
{
  thiz->drawPath(path->detach(), *paint);
}
#define NQCanvasContext_drawPath NQCanvasContext_drawPath

static inline void NQCanvasContext_drawRect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h, NQCanvasPaint* paint)
{
  thiz->drawRect(SkRect::MakeXYWH(x, y, w, h), *paint);
}
#define NQCanvasContext_drawRect NQCanvasContext_drawRect

static inline void NQCanvasContext_drawRoundRect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h, NQCanvasFloat r, NQCanvasPaint* paint)
{
  thiz->drawRoundRect(SkRect::MakeXYWH(x, y, w, h), r, r, *paint);
}
#define NQCanvasContext_drawRoundRect NQCanvasContext_drawRoundRect

static inline void NQCanvasContext_drawEllipse(NQCanvasContext* thiz, NQCanvasPaint* paint, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  thiz->drawOval(SkRect::MakeXYWH(x, y, w, h), *paint);
}
#define NQCanvasContext_drawEllipse NQCanvasContext_drawEllipse

#ifdef __cplusplus
}
#endif

#endif /* CANVAS_BACKEND_SKIA */
#endif /* _LIBNETQ_GFX_SKIA_CANVAS_H */
