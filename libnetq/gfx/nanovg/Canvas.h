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
#include <libnetq/Malloc.h>
#include <libnetq/string/String.h>

#include <nanovg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_CANVAS_BACKEND_NAME "nanovg"

typedef struct NQCanvas NQCanvas;
struct NQCanvas {
  NVGcontext* context;
  NQTextState text;
};

typedef NVGcontext NQCanvasContext;
typedef NVGcontext NQCanvasPath;
typedef int NQCanvasLineCap;
typedef int NQCanvasLineJoin;
typedef float NQCanvasFloat;
typedef void NQCanvasFontFace;

typedef struct NQCanvasFontMetrics {
  NQCanvasFloat ascent;
  NQCanvasFloat descent;
} NQCanvasFontMetrics;

/* A NanoVG image handle cast to a pointer, so NULL is no image: NanoVG never
 * hands out handle 0. It belongs to the context that made it. */
typedef void NQCanvasImage;

#define toCanvasFloat(value) ((float)(value))
#define toCanvasImage(handle) ((NQCanvasImage*)(intptr_t)(handle))
#define fromCanvasImage(image) ((int)(intptr_t)(image))
#define getCanvasContext(thiz) (thiz)->context
#define getCanvasTextState(thiz) (&(thiz)->text)

static inline void NQCanvasContext_imageSize(NQCanvasContext* thiz, NQCanvasImage* image, int* w, int* h)
{
  *w = 0;
  *h = 0;
  nvgImageSize(thiz, fromCanvasImage(image), w, h);
}
#define NQCanvasContext_imageSize NQCanvasContext_imageSize

static inline void NQCanvasContext_release(NQCanvasContext* thiz)
{
  nvgDeleteInternal(thiz);
}

static inline void NQCanvasContext_save(NQCanvasContext* thiz)
{
  nvgSave(thiz);
}
#define NQCanvasContext_save NQCanvasContext_save

static inline void NQCanvasContext_restore(NQCanvasContext* thiz)
{
  nvgRestore(thiz);
}
#define NQCanvasContext_restore NQCanvasContext_restore

static inline void NQCanvasContext_clipRect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  nvgIntersectScissor(thiz, x, y, w, h);
}
#define NQCanvasContext_clipRect NQCanvasContext_clipRect

static inline void NQCanvasContext_translate(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y)
{
  nvgTranslate(thiz, x, y);
}
#define NQCanvasContext_translate NQCanvasContext_translate

static inline void NQCanvasContext_scale(NQCanvasContext* thiz, NQCanvasFloat sx, NQCanvasFloat sy)
{
  nvgScale(thiz, sx, sy);
}
#define NQCanvasContext_scale NQCanvasContext_scale

static inline void NQCanvasContext_rotate(NQCanvasContext* thiz, NQCanvasFloat angle)
{
  nvgRotate(thiz, angle);
}
#define NQCanvasContext_rotate NQCanvasContext_rotate

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

static inline NQCanvasLineJoin toCanvasLineJoin(NQLineJoin lineJoin)
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

static inline void NQCanvasContext_setLineJoin(NQCanvasContext* thiz, NQCanvasLineJoin lineJoin)
{
  nvgLineJoin(thiz, lineJoin);
}
#define NQCanvasContext_setLineJoin NQCanvasContext_setLineJoin

static inline void NQCanvasContext_setMiterLimit(NQCanvasContext* thiz, NQCanvasFloat miterLimit)
{
  nvgMiterLimit(thiz, miterLimit);
}
#define NQCanvasContext_setMiterLimit NQCanvasContext_setMiterLimit

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

static inline void NQCanvasContext_arcTo(NQCanvasContext* thiz, NQCanvasFloat x1, NQCanvasFloat y1, NQCanvasFloat x2, NQCanvasFloat y2, NQCanvasFloat radius)
{
  nvgArcTo(thiz, x1, y1, x2, y2, radius);
}
#define NQCanvasContext_arcTo NQCanvasContext_arcTo

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

static inline void NQCanvasContext_ellipse(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  nvgEllipse(thiz, x + w / 2, y + h / 2, w / 2, h / 2);
}
#define NQCanvasContext_ellipse NQCanvasContext_ellipse

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

static inline int toCanvasFontFaceIndex(const NQCanvasFontFace* fontFace)
{
  return (int)((uintptr_t)fontFace - 1);
}

static inline NQCanvasFontFace* NQCanvasContext_fontFaceFromFile(NQCanvasContext* thiz, const char* filename, size_t index)
{
  int id = nvgCreateFontAtIndex(thiz, filename, filename, index);
  return id < 0 ? NULL : (NQCanvasFontFace*)((uintptr_t)id + 1);
}

static inline void NQCanvasContext_setFontFace(NQCanvasContext* thiz, NQCanvasFontFace* fontFace)
{
  nvgFontFaceId(thiz, toCanvasFontFaceIndex(fontFace));
  nvgTextAlign(thiz, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
}
#define NQCanvasContext_setFontFace NQCanvasContext_setFontFace

static inline void NQCanvasContext_setFontSize(NQCanvasContext* thiz, NQCanvasFloat fontSize)
{
  nvgFontSize(thiz, fontSize);
}
#define NQCanvasContext_setFontSize NQCanvasContext_setFontSize

static inline void NQCanvasContext_fontMetrics(NQCanvasContext* thiz, NQCanvasFontMetrics* fontMetrics)
{
  fontMetrics->ascent = 0;
  fontMetrics->descent = 0;
  nvgTextMetrics(thiz, &fontMetrics->ascent, &fontMetrics->descent, NULL);
}
#define NQCanvasContext_fontMetrics NQCanvasContext_fontMetrics

static inline NQCanvasFloat NQCanvasFontMetrics_ascent(const NQCanvasFontMetrics* thiz)
{
  return thiz->ascent;
}

static inline NQCanvasFloat NQCanvasFontMetrics_descent(const NQCanvasFontMetrics* thiz)
{
  return (0 - thiz->descent);
}

static inline NQCanvasFloat NQCanvasContext_textWidth(NQCanvasContext* thiz, const char* text, size_t length)
{
  return nvgTextBounds(thiz, 0, 0, text, text + length, NULL);
}
#define NQCanvasContext_textWidth NQCanvasContext_textWidth

static inline void NQCanvasContext_fillText(NQCanvasContext* thiz, const char* text, size_t length, NQCanvasFloat x, NQCanvasFloat y)
{
  /* the canvas has already moved (x, y) to where its own align puts it */
  nvgTextAlign(thiz, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
  nvgText(thiz, x, y, text, text + length);
}
#define NQCanvasContext_fillText NQCanvasContext_fillText

static inline NQCanvasImage* NQCanvasContext_createImageRGBA(NQCanvasContext* thiz, const void* pixels, int width, int height, int stride)
{
  /* NanoVG takes the rows packed back to back */
  const unsigned char* data = (const unsigned char*)pixels;
  unsigned char* packed = NULL;
  const size_t rowBytes = (size_t)width * 4;
  if ((size_t)stride != rowBytes) {
    packed = (unsigned char*)NQMalloc(rowBytes * (size_t)height);
    if (!packed)
      return NULL;
    for (int y = 0; y < height; y++)
      NQMemcpy(packed + rowBytes * (size_t)y, data + (size_t)stride * (size_t)y, rowBytes);
    data = packed;
  }

  const int handle = nvgCreateImageRGBA(thiz, width, height, 0, data);
  NQFree(packed);
  return toCanvasImage(handle);
}
#define NQCanvasContext_createImageRGBA NQCanvasContext_createImageRGBA

static inline void NQCanvasContext_deleteImage(NQCanvasContext* thiz, NQCanvasImage* image)
{
  nvgDeleteImage(thiz, fromCanvasImage(image));
}
#define NQCanvasContext_deleteImage NQCanvasContext_deleteImage

static inline void NQCanvasContext_drawImage(NQCanvasContext* thiz, NQCanvasImage* image,
    NQCanvasFloat sx, NQCanvasFloat sy, NQCanvasFloat sw, NQCanvasFloat sh,
    NQCanvasFloat dx, NQCanvasFloat dy, NQCanvasFloat dw, NQCanvasFloat dh)
{
  /* the pattern spans the whole image, scaled so the source rect lands on
   * the destination; the rect then cuts out just that part */
  const NQCanvasFloat ax = dw / sw;
  const NQCanvasFloat ay = dh / sh;

  int width, height;
  NQCanvasContext_imageSize(thiz, image, &width, &height);
  NVGpaint paint = nvgImagePattern(thiz, dx - sx * ax, dy - sy * ay, width * ax, height * ay, 0, fromCanvasImage(image), 1.0f);

  /* NanoVG has no way to keep a path aside, so unlike the other backends
   * this one does clear the current path */
  nvgSave(thiz); /* keeps the color fill set by NQCanvas_setColor */
  nvgBeginPath(thiz);
  nvgRect(thiz, dx, dy, dw, dh);
  nvgFillPaint(thiz, paint);
  nvgFill(thiz);
  nvgRestore(thiz);
}
#define NQCanvasContext_drawImage NQCanvasContext_drawImage

NQ_EXPORT NQCanvas* NQCanvas_fromNanovg(NVGcontext* context);

#ifdef __cplusplus
}
#endif

#endif /* CANVAS_BACKEND_NANOVG */
#endif /* _LIBNETQ_GFX_NANOVG_CANVAS_H */
