/*
 * MIT License
 *
 * Copyright (c) 2024-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/gfx/Image.h"

#include <libnetq/Malloc.h>

NQImage* NQImage_alloc(size_t size)
{
  NQImage* thiz;
  thiz = NQMalloc(sizeof(*thiz) - sizeof(thiz->data) + size);
  return thiz;
}

NQImage* NQImage_create(NQColorSpace colorspace, uint32_t width, uint32_t height, uint32_t stride)
{
  NQImage* thiz = NQImage_alloc(height * stride);
  if (thiz == NULL) {
    return NULL;
  }

  thiz->colorspace = colorspace;
  thiz->width = width;
  thiz->height = height;
  thiz->stride = stride;

  return thiz;
}

void NQImage_destroy(NQImage* thiz)
{
  NQFree(thiz);
}
