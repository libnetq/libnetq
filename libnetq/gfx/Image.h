/*
 * MIT License
 *
 * Copyright (c) 2024-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_GFX_IMAGE_H
#define _LIBNETQ_GFX_IMAGE_H

#include <libnetq/Basic.h>
#include <libnetq/gfx/ColorSpace.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQImageInfo NQImageInfo;
struct NQImageInfo {
  NQColorSpace colorspace;
  uint32_t width;
  uint32_t height;
  uint32_t stride;
};

typedef struct NQImageStorage NQImageStorage;
struct NQImageStorage {
  NQColorSpace colorspace;
  uint32_t width;
  uint32_t height;
  uint32_t stride;
  const uint8_t* data;
};

#define NQ_YUVA_Y_IDX 0
#define NQ_YUVA_U_IDX 1
#define NQ_YUVA_V_IDX 2
#define NQ_YUVA_A_IDX 3

typedef struct NQImageStorage4 NQImageStorage4;
struct NQImageStorage4 {
  NQColorSpace colorspace;
  uint32_t width;
  uint32_t height;
  uint32_t stride[4];
  const uint8_t* data[4];
};

typedef struct NQImage NQImage;
struct NQImage {
  NQColorSpace colorspace;
  uint32_t width;
  uint32_t height;
  uint32_t stride;
  uint8_t data[1];
};

NQ_EXPORT NQImage* NQImage_alloc(size_t size);
NQ_EXPORT NQImage* NQImage_create(NQColorSpace colorspace, uint32_t width, uint32_t height, uint32_t stride);
NQ_EXPORT void NQImage_destroy(NQImage*);

static inline NQColorSpace NQImage_colorspace(const NQImage* thiz)
{
  return thiz->colorspace;
}

static inline uint32_t NQImage_width(const NQImage* thiz)
{
  return thiz->width;
}

static inline uint32_t NQImage_height(const NQImage* thiz)
{
  return thiz->height;
}

static inline uint32_t NQImage_stride(const NQImage* thiz)
{
  return thiz->stride;
}

#define NQImage_data(thiz) (thiz)->data

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_GFX_IMAGE_H */
