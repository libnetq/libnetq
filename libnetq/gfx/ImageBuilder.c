/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/gfx/ImageBuilder.h"

#include <libnetq/Assert.h>
#include <libnetq/String.h>
#include <libnetq/Malloc.h>

struct NQImageBuilderPriv {
  NQImageBuilder base;
  uint8_t* current;
  uint8_t* end;
  uint32_t lineSize;
  uint32_t paddingSize;
};

NQImageBuilder* NQImageBuilder_create(NQColorSpace colorspace, uint32_t width, uint32_t height)
{
  uint32_t lineSize;
  switch (colorspace) {
  case kNQColorSpaceMonochrome:
    lineSize = (width + 7) / 8;
    break;
  case kNQColorSpaceRGB:
  case kNQColorSpaceBGR:
    lineSize = width * 3;
    break;
  case kNQColorSpaceRGBA:
  case kNQColorSpaceBGRA:
    lineSize = width * 4;
    break;
  default:
    return NULL;
  }

  uint32_t stride = lineSize;
  if (stride < lineSize) {
    return NULL;
  }

  size_t imageSize = height * stride;
  struct NQImageBuilderPriv* thiz = (struct NQImageBuilderPriv*)NQMalloc(sizeof(struct NQImageBuilderPriv) + imageSize);
  thiz->base.colorspace = colorspace;
  thiz->base.width = width;
  thiz->base.height = height;
  thiz->base.stride = stride;
  thiz->base.data = (uint8_t*)thiz + sizeof(struct NQImageBuilderPriv);
  thiz->current = thiz->base.data;
  thiz->end = thiz->current + imageSize;
  thiz->lineSize = lineSize;
  thiz->paddingSize = stride - lineSize;

  return &thiz->base;
}

void NQImageBuilder_destroy(NQImageBuilder* builder)
{
  struct NQImageBuilderPriv* thiz = (struct NQImageBuilderPriv*)builder;
  NQFree(thiz);
}

bool NQImageBuilder_addLine(NQImageBuilder* builder, const uint8_t* line)
{
  struct NQImageBuilderPriv* thiz = (struct NQImageBuilderPriv*)builder;
  if (thiz->end <= thiz->current) {
    return false;
  }

  memcpy(thiz->current, line, thiz->lineSize);
  thiz->current += thiz->lineSize;

  if (thiz->paddingSize != 0) {
    memset(thiz->current, 0, thiz->paddingSize);
    thiz->current += thiz->paddingSize;
  }

  return true;
}

bool NQImageBuilder_finish(NQImageBuilder* builder)
{
  struct NQImageBuilderPriv* thiz = (struct NQImageBuilderPriv*)builder;
  return (thiz->end == thiz->current) ? true : false;
}

struct NQYUVImageBuilderPriv {
  NQYUVImageBuilder base;
  uint8_t* currY;
  uint8_t* currU;
  uint8_t* currV;
  uint32_t y;
};

NQYUVImageBuilder* NQYUVImageBuilder_create(uint32_t width, uint32_t height)
{
  size_t size = width * height;

  uint32_t width2 = (width + 1) / 2;
  uint32_t height2 = (height + 1) / 2;
  uint32_t size2 = width2 * height2;

  struct NQYUVImageBuilderPriv* thiz = (struct NQYUVImageBuilderPriv*)NQMalloc(sizeof(struct NQYUVImageBuilderPriv) + size + size2 * 2);
  uint8_t* data = (uint8_t*)thiz + sizeof(struct NQYUVImageBuilderPriv);

  thiz->base.width = width;
  thiz->base.height = height;

  thiz->base.sizeY = size;
  thiz->base.sizeU = size2;
  thiz->base.sizeV = size2;

  thiz->base.strideY = width;
  thiz->base.strideU = width2;
  thiz->base.strideV = width2;

  thiz->base.dataY = data;
  thiz->base.dataU = data + size;
  thiz->base.dataV = data + size + size2;

  thiz->currY = thiz->base.dataY;
  thiz->currU = thiz->base.dataU;
  thiz->currV = thiz->base.dataV;

  thiz->y = 0;

  return &thiz->base;
}

void NQYUVImageBuilder_destroy(NQYUVImageBuilder* builder)
{
  struct NQYUVImageBuilderPriv* thiz = (struct NQYUVImageBuilderPriv*)builder;
  NQFree(thiz);
}

bool NQYUVImageBuilder_addLine(NQYUVImageBuilder* builder, const uint8_t* line)
{
  struct NQYUVImageBuilderPriv* thiz = (struct NQYUVImageBuilderPriv*)builder;
  if (thiz->base.dataU <= thiz->currY) {
    return false;
  }

  if ((thiz->y % 2) == 0) {
    for (uint32_t x = 0; x < thiz->base.width; x++) {
      *thiz->currY++ = line[0];
      if ((x % 2) == 0) {
        *thiz->currU++ = line[1];
        *thiz->currV++ = line[2];
      }
      line += 3;
    }
  }
  else {
    for (uint32_t x = 0; x < thiz->base.width; x++) {
      *thiz->currY++ = line[0];
      line += 3;
    }
  }

  thiz->y++;
  return true;
}

bool NQYUVImageBuilder_finish(NQYUVImageBuilder* builder)
{
  struct NQYUVImageBuilderPriv* thiz = (struct NQYUVImageBuilderPriv*)builder;
  NQ_ASSERT(thiz->currY == thiz->base.dataU && thiz->currU == thiz->base.dataV);
  return (thiz->y == thiz->base.height) ? true : false;
}
