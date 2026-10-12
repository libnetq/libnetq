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
  /* glyphs at fractional positions, so text lines up with NQCanvas_textWidth */
  thiz->font.setSubpixel(true);
  thiz->font.setEdging(SkFont::Edging::kAntiAlias);
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
  if (!thiz)
    return;
  thiz->sk.reset(); /* before the pixels it paints into */
  NQFree(thiz->owned_pixels);
  delete thiz;
}

#endif /* CANVAS_BACKEND_SKIA */
