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
#include "libnetq/gfx/skia/Canvas.h"

#ifdef CANVAS_BACKEND_SKIA

#include <libnetq/Malloc.h>
#include <libnetq/string/String.h>

NQCanvas* NQCanvas_createForData(void* pixels, int width, int height, int stride)
{
  if (!pixels || width <= 0 || height <= 0)
    return NULL;

  /* BGRA bytes are 0xAARRGGBB words on the little-endian targets */
  SkImageInfo info = SkImageInfo::Make(width, height, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
  std::unique_ptr<SkCanvas> sk = SkCanvas::MakeRasterDirect(info, pixels, (size_t)stride);
  if (!sk)
    return NULL;

  NQCanvas* thiz = new NQCanvas();
  thiz->sk = std::move(sk);
  thiz->owned_pixels = NULL;
  thiz->paint.setAntiAlias(true);
  thiz->paint.setStrokeWidth(1);
  thiz->paint.setStrokeCap(SkPaint::kButt_Cap);
  thiz->paint.setStrokeJoin(SkPaint::kMiter_Join);
  thiz->font.setSize(12);
  return thiz;
}

NQCanvas* NQCanvas_create(int width, int height)
{
  if (width <= 0 || height <= 0)
    return NULL;
  int stride = width * 4;
  void* pixels = NQZalloc((size_t)(height * stride));
  if (!pixels)
    return NULL;
  NQCanvas* thiz = NQCanvas_createForData(pixels, width, height, stride);
  if (!thiz) {
    NQFree(pixels);
    return NULL;
  }
  thiz->owned_pixels = pixels;
  return thiz;
}

void NQCanvas_release(NQCanvas* thiz)
{
  thiz->sk.reset(); /* before the pixels it paints into */
  NQFree(thiz->owned_pixels);
  delete thiz;
}

void NQCanvas_setFont(NQCanvas* thiz, const NQGFont* font)
{
  SkScalar size = thiz->font.getSize();
  thiz->font = *font;
  thiz->font.setSize(size);
}

void NQCanvas_setFontSize(NQCanvas* thiz, double size)
{
  thiz->font.setSize((SkScalar)size);
}

NQGFontMetrics NQCanvas_fontMetrics(NQCanvas* thiz)
{
  SkFontMetrics m;
  thiz->font.getMetrics(&m);

  /* Skia reports the ascent above the baseline as a negative number */
  NQGFontMetrics result;
  result.ascent = (0 - m.fAscent);
  result.descent = m.fDescent;

  return result;
}

double NQCanvas_textWidth(NQCanvas* thiz, const char* text)
{
  return NQCanvas_textWidth2(thiz, text, text ? NQStrlen(text) : 0);
}

double NQCanvas_textWidth2(NQCanvas* thiz, const char* text, size_t length)
{
  return thiz->font.measureText(text, length, SkTextEncoding::kUTF8);
}

void NQCanvas_drawText2(NQCanvas* thiz, const char* text, size_t length, double x, double baseline)
{
  thiz->paint.setStroke(false);
  thiz->sk->drawSimpleText(text, length, SkTextEncoding::kUTF8, (SkScalar)x, (SkScalar)baseline, thiz->font, thiz->paint);
}

void NQCanvas_drawText(NQCanvas* thiz, const char* text, double x, double baseline)
{
  NQCanvas_drawText2(thiz, text, text ? NQStrlen(text) : 0, x, baseline);
}

#endif /* CANVAS_BACKEND_SKIA */
