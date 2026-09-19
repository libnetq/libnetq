/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_GFX_COLORUTILS_H
#define _LIBNETQ_GFX_COLORUTILS_H

#include <libnetq/Basic.h>
#include <libnetq/gfx/ColorSpace.h>
#include <libnetq/Limits.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline uint8_t NQRgbToGrayI(uint8_t r, uint8_t g, uint8_t b)
{
  return (uint8_t)((4899u * r + 9617u * g + 1868u * b) >> 14);
}

static inline void NQCmykToRgbI(uint8_t c, uint8_t m, uint8_t y, uint8_t k, uint8_t* r, uint8_t* g, uint8_t* b)
{
  uint8_t C = NQ_UINT8_MAX - c;
  uint8_t M = NQ_UINT8_MAX - m;
  uint8_t Y = NQ_UINT8_MAX - y;
  uint8_t K = NQ_UINT8_MAX - k;

  *r = (uint8_t)(C * K / NQ_UINT8_MAX);
  *g = (uint8_t)(M * K / NQ_UINT8_MAX);
  *b = (uint8_t)(Y * K / NQ_UINT8_MAX);
}

static inline void NQInvertedCmykToRgbI(uint8_t c, uint8_t m, uint8_t y, uint8_t k, uint8_t* r, uint8_t* g, uint8_t* b)
{
  *r = (uint8_t)(((unsigned)c * (unsigned)k + 127) / NQ_UINT8_MAX);
  *g = (uint8_t)(((unsigned)m * (unsigned)k + 127) / NQ_UINT8_MAX);
  *b = (uint8_t)(((unsigned)y * (unsigned)k + 127) / NQ_UINT8_MAX);
}

#ifdef NQ_USE_FLOATING_POINT
static inline uint8_t NQRgbToGrayF(uint8_t r, uint8_t g, uint8_t b)
{
  return (uint8_t)(0.299f * r + 0.587f * g + 0.114f * b);
}

static inline void NQCmykToRgbF(uint8_t c, uint8_t m, uint8_t y, uint8_t k, uint8_t* r, uint8_t* g, uint8_t* b)
{
  double C = 1.0 - c / (double)NQ_UINT8_MAX;
  double M = 1.0 - m / (double)NQ_UINT8_MAX;
  double Y = 1.0 - y / (double)NQ_UINT8_MAX;
  double K = 1.0 - k / (double)NQ_UINT8_MAX;

  *r = (uint8_t)((double)NQ_UINT8_MAX * C * K);
  *g = (uint8_t)((double)NQ_UINT8_MAX * M * K);
  *b = (uint8_t)((double)NQ_UINT8_MAX * Y * K);
}

static inline void NQInvertedCmykToRgbF(uint8_t c, uint8_t m, uint8_t y, uint8_t k, uint8_t* r, uint8_t* g, uint8_t* b)
{
  *r = (uint8_t)((double)c * (double)k / (double)NQ_UINT8_MAX + 0.5);
  *g = (uint8_t)((double)m * (double)k / (double)NQ_UINT8_MAX + 0.5);
  *b = (uint8_t)((double)y * (double)k / (double)NQ_UINT8_MAX + 0.5);
}

#define NQRgbToGray NQRgbToGrayF
#define NQCmykToRgb NQCmykToRgbF
#define NQInvertedCmykToRgb NQInvertedCmykToRgbF

#else
#define NQRgbToGray NQRgbToGrayI
#define NQCmykToRgb NQCmykToRgbI
#define NQInvertedCmykToRgb NQInvertedCmykToRgbI
#endif

static inline uint32_t NQGetPixelSize(NQColorSpace colorspace)
{
  switch (colorspace) {
  case kNQColorSpaceNone:
    return 0;
  case kNQColorSpaceMonochrome:
    return 1;
  case kNQColorSpaceGray:
    return 1;
  case kNQColorSpaceRGB:
    return 3;
  case kNQColorSpaceRGBA:
    return 4;
  case kNQColorSpaceRGBX:
    return 4;
  case kNQColorSpaceXRGB:
    return 4;
  case kNQColorSpaceBGR:
    return 3;
  case kNQColorSpaceBGRA:
    return 4;
  case kNQColorSpaceBGRX:
    return 4;
  case kNQColorSpaceXBGR:
    return 4;
  case kNQColorSpaceCMYK:
    return 4;
  case kNQColorSpaceYUV:
    return 3;
  }
  return 0;
}

static inline uint32_t NQGetPixelBitDepth(NQColorSpace colorspace)
{
  switch (colorspace) {
  case kNQColorSpaceNone:
    return 0;
  case kNQColorSpaceMonochrome:
    return 1;
  case kNQColorSpaceGray:
    return 8;
  case kNQColorSpaceRGB:
    return 24;
  case kNQColorSpaceRGBA:
    return 32;
  case kNQColorSpaceRGBX:
    return 32;
  case kNQColorSpaceXRGB:
    return 32;
  case kNQColorSpaceBGR:
    return 24;
  case kNQColorSpaceBGRA:
    return 32;
  case kNQColorSpaceBGRX:
    return 32;
  case kNQColorSpaceXBGR:
    return 32;
  case kNQColorSpaceCMYK:
    return 32;
  case kNQColorSpaceYUV:
    return 24;
  }
  return 0;
}

NQ_EXPORT uint32_t NQCalcColorLineSize(NQColorSpace colorspace, uint32_t width);

NQ_EXPORT void NQRgbToGrayLine(uint32_t width, const uint8_t* in, uint8_t* out);
NQ_EXPORT void NQRgbaToGrayLine(uint32_t width, const uint8_t* in, uint8_t* out);
NQ_EXPORT void NQBgraToGrayLine(uint32_t width, const uint8_t* in, uint8_t* out);
NQ_EXPORT void NQBgrxToGrayLine(uint32_t width, const uint8_t* in, uint8_t* out);
NQ_EXPORT void NQGrayToRgbLine(uint32_t width, const uint8_t* in, uint8_t* out);
NQ_EXPORT void NQGrayToBgrLine(uint32_t width, const uint8_t* in, uint8_t* out);
NQ_EXPORT void NQGrayToMonochromeLine(uint32_t width, const uint8_t* in, uint8_t* out);
NQ_EXPORT void NQRgbaToRgbLine(uint32_t width, const uint8_t* in, uint8_t* out);
NQ_EXPORT void NQBgraToBgrLine(uint32_t width, const uint8_t* in, uint8_t* out);
NQ_EXPORT void NQRgbxToRgbLine(uint32_t width, const uint8_t* in, uint8_t* out);
NQ_EXPORT void NQBgrxToBgrLine(uint32_t width, const uint8_t* in, uint8_t* out);
NQ_EXPORT void NQRgbaToBgrLine(uint32_t width, const uint8_t* in, uint8_t* out);
NQ_EXPORT void NQBgraToRgbLine(uint32_t width, const uint8_t* in, uint8_t* out);
NQ_EXPORT void NQRgbxToBgrLine(uint32_t width, const uint8_t* in, uint8_t* out);
NQ_EXPORT void NQBgrxToRgbLine(uint32_t width, const uint8_t* in, uint8_t* out);
NQ_EXPORT void NQRgbToBgrLine(uint32_t width, const uint8_t* in, uint8_t* out);
NQ_EXPORT void NQBgrToRgbLine(uint32_t width, const uint8_t* in, uint8_t* out);
NQ_EXPORT void NQMonochromeToGrayLine(uint32_t width, const uint8_t* in, uint8_t* out);
NQ_EXPORT void NQCmykToRgbLine(uint32_t width, const uint8_t* in, uint8_t* out);
NQ_EXPORT void NQInvertedCmykToRgbLine(uint32_t width, const uint8_t* in, uint8_t* out);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_GFX_COLORUTILS_H */
