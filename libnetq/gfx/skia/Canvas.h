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
#include "include/core/SkFontMgr.h"
#include "include/core/SkImage.h"
#include "include/core/SkPixmap.h"
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
typedef SkPaint::Join NQCanvasLineJoin;
typedef SkPathBuilder NQCanvasPath;
typedef SkFontMgr NQCanvasFontFaceCache;
typedef SkTypeface NQCanvasFontFace;
typedef SkFont NQCanvasFont;
typedef SkFontMetrics NQCanvasFontMetrics;
typedef SkFontStyle::Slant NQCanvasFontSlant;
typedef SkFontStyle::Weight NQCanvasFontWeight;
typedef SkScalar NQCanvasFloat;

#define toCanvasFloat(value) static_cast<SkScalar>(value)
#define getCanvasContext(canvas) (canvas)->sk.get()
#define getCanvasPaint(canvas) &(canvas)->paint
#define getCanvasPath(canvas) &(canvas)->path
#define getCanvasFont(canvas) &(canvas)->font
#define getCanvasTextState(canvas) (&(canvas)->text)

struct NQCanvas {
  std::unique_ptr<NQCanvasContext> sk;
  void* owned_pixels;
  NQCanvasPaint paint;
  NQCanvasPath path;
  NQCanvasFont font;
  NQTextState text;
};

static inline NQCanvasFontSlant toCanvasFontSlant(NQFontSlant fontSlant)
{
  switch (fontSlant) {
  case NQ_FONT_SLANT_NORMAL:
    return SkFontStyle::kUpright_Slant;
  case NQ_FONT_SLANT_ITALIC:
    return SkFontStyle::kItalic_Slant;
  case NQ_FONT_SLANT_OBLIQUE:
    return SkFontStyle::kOblique_Slant;
  }
  NQ_ASSERT_NOT_REACHED();
  return SkFontStyle::kUpright_Slant;
}

static inline NQCanvasFontWeight toCanvasFontWeight(NQFontWeight fontWeight)
{
  switch (fontWeight) {
  case NQ_FONT_WEIGHT_NORMAL:
    return SkFontStyle::kNormal_Weight;
  case NQ_FONT_WEIGHT_BOLD:
    return SkFontStyle::kBold_Weight;
  }
  NQ_ASSERT_NOT_REACHED();
  return SkFontStyle::kNormal_Weight;
}

static inline NQCanvasFontFace* NQCanvasFontFaceCache_load(NQCanvasFontFaceCache* thiz, const char* filename, size_t index)
{
  return thiz->makeFromFile(filename, index).release();
}

static inline int NQCanvasImage_width(const NQCanvasImage* image)
{
  return image->width();
}
#define NQCanvasImage_width NQCanvasImage_width

static inline int NQCanvasImage_height(const NQCanvasImage* image)
{
  return image->height();
}
#define NQCanvasImage_height NQCanvasImage_height

static inline void NQCanvasContext_save(NQCanvasContext* thiz)
{
  thiz->save();
}
#define NQCanvasContext_save NQCanvasContext_save

static inline void NQCanvasContext_restore(NQCanvasContext* thiz)
{
  thiz->restore();
}
#define NQCanvasContext_restore NQCanvasContext_restore

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
#define NQCanvasContext_translate NQCanvasContext_translate

static inline void NQCanvasContext_scale(NQCanvasContext* thiz, NQCanvasFloat sx, NQCanvasFloat sy)
{
  thiz->scale(sx, sy);
}
#define NQCanvasContext_scale NQCanvasContext_scale

static inline void NQCanvasContext_rotate(NQCanvasContext* thiz, NQCanvasFloat angle)
{
  thiz->rotate(SkRadiansToDegrees(angle));
}
#define NQCanvasContext_rotate NQCanvasContext_rotate

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

static inline NQCanvasLineJoin toCanvasLineJoin(NQLineJoin lineJoin)
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

static inline void NQCanvasPaint_setLineJoin(NQCanvasPaint* thiz, NQCanvasLineJoin lineJoin)
{
  thiz->setStrokeJoin(lineJoin);
}
#define NQCanvasPaint_setLineJoin NQCanvasPaint_setLineJoin

static inline void NQCanvasPaint_setMiterLimit(NQCanvasPaint* thiz, NQCanvasFloat miterLimit)
{
  thiz->setStrokeMiter(miterLimit);
}
#define NQCanvasPaint_setMiterLimit NQCanvasPaint_setMiterLimit

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

static inline void NQCanvasPath_arcTo(NQCanvasPath* thiz, NQCanvasFloat x1, NQCanvasFloat y1, NQCanvasFloat x2, NQCanvasFloat y2, NQCanvasFloat radius)
{
  thiz->arcTo(SkPoint::Make(x1, y1), SkPoint::Make(x2, y2), radius);
}
#define NQCanvasPath_arcTo NQCanvasPath_arcTo

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

static inline void NQCanvasFontFace_release(NQCanvasFontFace* fontFace)
{
  SkSafeUnref(fontFace);
}
#define NQCanvasFontFace_release NQCanvasFontFace_release

static inline void NQCanvasFont_setFace(NQCanvasFont* thiz, NQCanvasFontFace* fontFace)
{
  thiz->setTypeface(sk_ref_sp(fontFace));
}
#define NQCanvasFont_setFace NQCanvasFont_setFace

static inline void NQCanvasFont_setSize(NQCanvasFont* thiz, NQCanvasFloat size)
{
  thiz->setSize(size);
}
#define NQCanvasFont_setSize NQCanvasFont_setSize

static inline void NQCanvasFont_metrics(NQCanvasFont* thiz, NQCanvasFontMetrics* fontMetrics)
{
  thiz->getMetrics(fontMetrics);
}
#define NQCanvasFont_metrics NQCanvasFont_metrics

static inline NQCanvasFloat NQCanvasFontMetrics_ascent(const NQCanvasFontMetrics* thiz)
{
  return (0 - thiz->fAscent);
}

static inline NQCanvasFloat NQCanvasFontMetrics_descent(const NQCanvasFontMetrics* thiz)
{
  return thiz->fDescent;
}

static inline NQCanvasFloat NQCanvasFont_textWidth(NQCanvasFont* thiz, const char* text, size_t length)
{
  return thiz->measureText(text, length, SkTextEncoding::kUTF8);
}
#define NQCanvasFont_textWidth NQCanvasFont_textWidth

static inline void NQCanvasContext_drawSimpleText(NQCanvasContext* thiz, const char* text, size_t length, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFont* font, NQCanvasPaint* paint)
{
  thiz->drawSimpleText(text, length, SkTextEncoding::kUTF8, x, y, *font, *paint);
}
#define NQCanvasContext_drawSimpleText NQCanvasContext_drawSimpleText

static inline NQCanvasImage* NQCanvasImage_createRGBA(const void* pixels, int width, int height, int stride)
{
  SkImageInfo info = SkImageInfo::Make(width, height, kRGBA_8888_SkColorType, kUnpremul_SkAlphaType);
  sk_sp<SkImage> image = SkImages::RasterFromPixmapCopy(SkPixmap(info, pixels, (size_t)stride));
  /* the caller takes over the reference, and NQCanvas_imageRelease drops it */
  return image.release();
}
#define NQCanvasImage_createRGBA NQCanvasImage_createRGBA

static inline void NQCanvasImage_release(NQCanvasImage* thiz)
{
  SkSafeUnref(thiz);
}
#define NQCanvasImage_release NQCanvasImage_release

static inline void NQCanvasContext_drawImage(NQCanvasContext* thiz, NQCanvasImage* image,
    NQCanvasFloat sx, NQCanvasFloat sy, NQCanvasFloat sw, NQCanvasFloat sh,
    NQCanvasFloat dx, NQCanvasFloat dy, NQCanvasFloat dw, NQCanvasFloat dh)
{
  /* strict, so sprites never sample their neighbours on the sheet */
  thiz->drawImageRect(image, SkRect::MakeXYWH(sx, sy, sw, sh), SkRect::MakeXYWH(dx, dy, dw, dh), SkSamplingOptions(SkFilterMode::kLinear), nullptr, SkCanvas::kStrict_SrcRectConstraint);
}
#define NQCanvasContext_drawImage NQCanvasContext_drawImage

#ifdef __cplusplus
}
#endif

#endif /* CANVAS_BACKEND_SKIA */
#endif /* _LIBNETQ_GFX_SKIA_CANVAS_H */
