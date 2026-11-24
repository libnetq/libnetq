/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/gfx/blend2d/Canvas.h"

#ifdef CANVAS_BACKEND_BLEND2D

#include <libnetq/gfx/Canvas.h>

/* the HTML canvas default, 10px */
#define DEFAULT_FONT_SIZE 10.0

/* Takes over the image, destroying it if the canvas cannot be made */
static NQCanvas* NQCanvas_fromImage(NQCanvasImage* image)
{
  NQCanvas* thiz = (NQCanvas*)NQMalloc(sizeof(NQCanvas));
  if (!thiz) {
    NQCanvasImage_finalize(image);
    return NULL;
  }
  thiz->image = *image;
  if (bl_context_init_as(&thiz->context, &thiz->image, NULL) != BL_SUCCESS) {
    bl_context_destroy(&thiz->context);
    NQCanvasImage_finalize(&thiz->image);
    NQFree(thiz);
    return NULL;
  }

  bl_path_init(&thiz->path);
  bl_font_init(&thiz->font);

  thiz->fontSize = DEFAULT_FONT_SIZE;
  thiz->text = (NQTextState){0};

  return thiz;
}

NQCanvas* NQCanvas_create(int width, int height)
{
  if (width <= 0 || height <= 0)
    return NULL;
  BLImageCore image;
  if (bl_image_init_as(&image, width, height, BL_FORMAT_PRGB32) != BL_SUCCESS) {
    NQCanvasImage_finalize(&image);
    return NULL;
  }
  NQCanvas* thiz = NQCanvas_fromImage(&image);
  /* a new image is uninitialized; start transparent, as the other backends do */
  if (thiz)
    bl_context_clear_all(&thiz->context);
  return thiz;
}

/* PRGB32 pixels are native-endian premultiplied 0xAARRGGBB words; the
 * pixels stay the caller's and must outlive the canvas */
NQCanvas* NQCanvas_createForData(void* pixels, int width, int height, int stride)
{
  if (!pixels || width <= 0 || height <= 0)
    return NULL;
  BLImageCore image;
  if (bl_image_init_as_from_data(&image, width, height, BL_FORMAT_PRGB32, pixels, stride, BL_DATA_ACCESS_RW, NULL, NULL) != BL_SUCCESS) {
    NQCanvasImage_finalize(&image);
    return NULL;
  }
  return NQCanvas_fromImage(&image);
}

void NQCanvas_release(NQCanvas* thiz)
{
  bl_context_destroy(&thiz->context); /* before the image it paints into */
  bl_path_destroy(&thiz->path);
  bl_font_destroy(&thiz->font);
  NQCanvasImage_finalize(&thiz->image);
  NQFree(thiz);
}

#endif /* CANVAS_BACKEND_BLEND2D */
