/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_GFX_COLORSPACE_H
#define _LIBNETQ_GFX_COLORSPACE_H

enum NQColorSpace {
  kNQColorSpaceNone,
  kNQColorSpaceMonochrome,
  kNQColorSpaceGray,
  kNQColorSpaceRGB,
  kNQColorSpaceRGBA,
  kNQColorSpaceRGBX,
  kNQColorSpaceXRGB,
  kNQColorSpaceBGR,
  kNQColorSpaceBGRA,
  kNQColorSpaceBGRX,
  kNQColorSpaceXBGR,
  kNQColorSpaceCMYK,
  kNQColorSpaceYUV, // YUV420
};

typedef enum NQColorSpace NQColorSpace;

#endif /* _LIBNETQ_GFX_COLORSPACE_H */
