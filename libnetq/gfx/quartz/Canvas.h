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
#include <libnetq/string/String.h>

#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_CANVAS_BACKEND_NAME "quartz"

typedef struct CGContext NQCanvasContext;
typedef NQCanvasContext NQCanvasPath;
typedef CGLineCap NQCanvasLineCap;
typedef CGLineJoin NQCanvasLineJoin;
typedef struct __CTFontDescriptor NQCanvasFontFace; // CTFontDescriptorRef
typedef struct __CTFont NQCanvasFont; // CTFontRef
typedef double NQCanvasFloat;

typedef struct NQCanvasFontMetrics {
  NQCanvasFloat ascent;
  NQCanvasFloat descent;
} NQCanvasFontMetrics;

typedef struct NQCanvas NQCanvas;
struct NQCanvas {
  CGContextRef context;
  const NQCanvasFontFace* fontDesc;
  CTFontRef font;
  CGFloat fontSize;
  NQTextState text;
};

/* NQCanvasImage* is a CGImageRef */
typedef struct CGImage NQCanvasImage;

#define toCanvasFloat(value) (value)
#define getCanvasContext(canvas) (canvas)->context
#define getCanvasFont(canvas) drawingFont(canvas)
#define getCanvasTextState(canvas) (&(canvas)->text)

/* CoreGraphics only reads the image, though its getters take a CGImageRef */
static inline int NQCanvasImage_width(NQCanvasImage* image)
{
  return (int)CGImageGetWidth(image);
}
#define NQCanvasImage_width NQCanvasImage_width

static inline int NQCanvasImage_height(NQCanvasImage* image)
{
  return (int)CGImageGetHeight(image);
}
#define NQCanvasImage_height NQCanvasImage_height

static inline void NQCanvasContext_release(NQCanvasContext* thiz)
{
  CGContextRelease(thiz);
}

static inline void NQCanvasContext_save(NQCanvasContext* thiz)
{
  CGContextSaveGState(thiz);
}
#define NQCanvasContext_save NQCanvasContext_save

static inline void NQCanvasContext_restore(NQCanvasContext* thiz)
{
  CGContextRestoreGState(thiz);
}
#define NQCanvasContext_restore NQCanvasContext_restore

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
#define NQCanvasContext_translate NQCanvasContext_translate

static inline void NQCanvasContext_scale(NQCanvasContext* thiz, NQCanvasFloat sx, NQCanvasFloat sy)
{
  CGContextScaleCTM(thiz, sx, sy);
}
#define NQCanvasContext_scale NQCanvasContext_scale

static inline void NQCanvasContext_rotate(NQCanvasContext* thiz, NQCanvasFloat angle)
{
  CGContextRotateCTM(thiz, angle);
}
#define NQCanvasContext_rotate NQCanvasContext_rotate

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

static inline NQCanvasLineJoin toCanvasLineJoin(NQLineJoin lineJoin)
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

static inline void NQCanvasContext_setLineJoin(NQCanvasContext* thiz, NQCanvasLineJoin lineJoin)
{
  CGContextSetLineJoin(thiz, lineJoin);
}
#define NQCanvasContext_setLineJoin NQCanvasContext_setLineJoin

static inline void NQCanvasContext_setMiterLimit(NQCanvasContext* thiz, NQCanvasFloat miterLimit)
{
  CGContextSetMiterLimit(thiz, miterLimit);
}
#define NQCanvasContext_setMiterLimit NQCanvasContext_setMiterLimit

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

static inline void NQCanvasContext_arcTo(NQCanvasContext* thiz, NQCanvasFloat x1, NQCanvasFloat y1, NQCanvasFloat x2, NQCanvasFloat y2, NQCanvasFloat radius)
{
  CGContextAddArcToPoint(thiz, x1, y1, x2, y2, radius);
}
#define NQCanvasContext_arcTo NQCanvasContext_arcTo

static inline void NQCanvasContext_rect(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  CGContextAddRect(thiz, CGRectMake(x, y, w, h));
}
#define NQCanvasContext_rect NQCanvasContext_rect

static inline void NQCanvasContext_ellipse(NQCanvasContext* thiz, NQCanvasFloat x, NQCanvasFloat y, NQCanvasFloat w, NQCanvasFloat h)
{
  CGContextAddEllipseInRect(thiz, CGRectMake(x, y, w, h));
}
#define NQCanvasContext_ellipse NQCanvasContext_ellipse

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

static inline CTFontRef drawingFont(NQCanvas* thiz)
{
  if (!thiz->font && thiz->fontDesc)
    thiz->font = CTFontCreateWithFontDescriptor(thiz->fontDesc, thiz->fontSize, NULL);
  return thiz->font;
}

static inline void forgetFont(NQCanvas* thiz)
{
  if (thiz->font)
    CFRelease(thiz->font);
  thiz->font = NULL;
}

static inline void NQCanvas_setFontSizeImpl(NQCanvas* thiz, NQCanvasFloat fontSize)
{
  if ((CGFloat)fontSize == thiz->fontSize)
    return;
  thiz->fontSize = (CGFloat)fontSize;
  forgetFont(thiz);
}
#define NQCanvas_setFontSizeImpl NQCanvas_setFontSizeImpl

static inline void NQCanvasFontFace_release(NQCanvasFontFace* thiz)
{
  CFRelease(thiz);
}
#define NQCanvasFontFace_release NQCanvasFontFace_release

static inline void NQCanvas_setFontFaceImpl(NQCanvas* thiz, NQCanvasFontFace* fontFace)
{
  if (fontFace == thiz->fontDesc)
    return;
  CFRetain(fontFace);
  if (thiz->fontDesc)
    CFRelease(thiz->fontDesc);
  thiz->fontDesc = fontFace;
  forgetFont(thiz);
}
#define NQCanvas_setFontFaceImpl NQCanvas_setFontFaceImpl

static inline void NQCanvas_fontMetricsImpl(NQCanvas* thiz, NQCanvasFontMetrics* fontMetrics)
{
  CTFontRef font = drawingFont(thiz);
  if (font) {
    fontMetrics->ascent = CTFontGetAscent(font);
    fontMetrics->descent = CTFontGetDescent(font);
  }
  else {
    fontMetrics->ascent = 0;
    fontMetrics->descent = 0;
  }
}
#define NQCanvas_fontMetricsImpl NQCanvas_fontMetricsImpl

static inline NQCanvasFloat NQCanvasFontMetrics_ascent(const NQCanvasFontMetrics* thiz)
{
  return thiz->ascent;
}

static inline NQCanvasFloat NQCanvasFontMetrics_descent(const NQCanvasFontMetrics* thiz)
{
  return thiz->descent;
}

double NQCanvas_textWidthImpl(NQCanvas* thiz, const char* text, size_t length);
#define NQCanvas_textWidthImpl NQCanvas_textWidthImpl

void NQCanvasFont_strokeText(NQCanvasContext* context, CTFontRef font, const char* text, size_t length, NQCanvasFloat x, NQCanvasFloat y);
#define NQCanvasFont_strokeText NQCanvasFont_strokeText

void NQCanvasFont_fillText(NQCanvasContext* context, CTFontRef font, const char* text, size_t length, NQCanvasFloat x, NQCanvasFloat y);
#define NQCanvasFont_fillText NQCanvasFont_fillText

static inline NQCanvasImage* NQCanvasImage_createRGBA(const void* pixels, int width, int height, int stride)
{
  /* the image keeps a packed copy, so the last row need not span a full stride */
  const size_t rowBytes = (size_t)width * 4;
  CFMutableDataRef data = CFDataCreateMutable(NULL, (CFIndex)(rowBytes * (size_t)height));
  if (!data)
    return NULL;

  CFDataSetLength(data, (CFIndex)(rowBytes * (size_t)height));
  UInt8* dst = CFDataGetMutableBytePtr(data);
  const UInt8* src = (const UInt8*)pixels;
  for (int y = 0; y < height; y++)
    NQMemcpy(dst + rowBytes * (size_t)y, src + (size_t)stride * (size_t)y, rowBytes);

  CGDataProviderRef provider = CGDataProviderCreateWithCFData(data);
  CFRelease(data);
  if (!provider)
    return NULL;

  CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
  CGImageRef image = CGImageCreate((size_t)width, (size_t)height, 8, 32, rowBytes, colorSpace,
      kCGImageAlphaLast | kCGBitmapByteOrderDefault, provider, NULL, true, kCGRenderingIntentDefault);
  CGColorSpaceRelease(colorSpace);
  CGDataProviderRelease(provider);
  return image;
}
#define NQCanvasImage_createRGBA NQCanvasImage_createRGBA

static inline void NQCanvasImage_release(NQCanvasImage* thiz)
{
  CGImageRelease(thiz);
}
#define NQCanvasImage_release NQCanvasImage_release

static inline void NQCanvasContext_drawImage(NQCanvasContext* thiz, NQCanvasImage* image,
    NQCanvasFloat sx, NQCanvasFloat sy, NQCanvasFloat sw, NQCanvasFloat sh,
    NQCanvasFloat dx, NQCanvasFloat dy, NQCanvasFloat dw, NQCanvasFloat dh)
{
  const NQCanvasFloat ax = dw / sw;
  const NQCanvasFloat ay = dh / sh;

  const int width = NQCanvasImage_width(image);
  const int height = NQCanvasImage_height(image);

  CGContextSaveGState(thiz);
  CGContextClipToRect(thiz, CGRectMake(dx, dy, dw, dh));
  /* the context is flipped, so the whole image is drawn upright with its
   * top-left at (dx - sx * ax, dy - sy * ay); the clip keeps the source rect */
  CGContextTranslateCTM(thiz, dx - sx * ax, dy - sy * ay + height * ay);
  CGContextScaleCTM(thiz, ax, -ay);
  CGContextDrawImage(thiz, CGRectMake(0, 0, width, height), image);
  CGContextRestoreGState(thiz);
}
#define NQCanvasContext_drawImage NQCanvasContext_drawImage

#ifdef __cplusplus
}
#endif

NQ_EXPORT NQCanvas* NQCanvas_fromCGContextRef(CGContextRef context);

#endif /* CANVAS_BACKEND_QUARTZ */
#endif /* _LIBNETQ_GFX_QUARTZ_CANVAS_H */
