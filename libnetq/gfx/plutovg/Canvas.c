/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/gfx/plutovg/Canvas.h"

#ifdef CANVAS_BACKEND_PLUTOVG

#include <libnetq/gfx/Canvas.h>

#include <libnetq/Malloc.h>

/* Takes over the surface, destroying it if the canvas cannot be made */
static NQCanvas* NQCanvas_fromSurface(plutovg_surface_t* surface)
{
  if (!surface)
    return NULL;
  NQCanvas* thiz = (NQCanvas*)NQMalloc(sizeof(NQCanvas));
  if (!thiz) {
    plutovg_surface_destroy(surface);
    return NULL;
  }
  thiz->surface = surface;
  thiz->canvas = plutovg_canvas_create(surface);
  thiz->text = (NQTextState){0};
  if (!thiz->canvas) {
    plutovg_surface_destroy(surface);
    NQFree(thiz);
    return NULL;
  }
  return thiz;
}

NQCanvas* NQCanvas_create(int width, int height)
{
  if (width <= 0 || height <= 0)
    return NULL;
  return NQCanvas_fromSurface(plutovg_surface_create(width, height));
}

/* PlutoVG pixels are native-endian premultiplied 0xAARRGGBB words; the
 * pixels stay the caller's and must outlive the canvas */
NQCanvas* NQCanvas_createForData(void* pixels, int width, int height, int stride)
{
  if (!pixels || width <= 0 || height <= 0)
    return NULL;
  return NQCanvas_fromSurface(plutovg_surface_create_for_data((unsigned char*)pixels, width, height, stride));
}

void NQCanvas_release(NQCanvas* thiz)
{
  if (!thiz)
    return;
  NQCanvasContext_release(thiz->canvas); /* before the surface it paints into */
  plutovg_surface_destroy(thiz->surface);
  NQFree(thiz);
}

#endif /* CANVAS_BACKEND_PLUTOVG */
