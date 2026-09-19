/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/gfx/ColorUtils.h"

uint32_t NQCalcColorLineSize(NQColorSpace colorspace, uint32_t width)
{
  switch (colorspace) {
  case kNQColorSpaceMonochrome:
    return (width + 7) / 8;
  case kNQColorSpaceGray:
    return width;
  case kNQColorSpaceRGB:
  case kNQColorSpaceBGR:
    return width * 3;
  case kNQColorSpaceRGBA:
  case kNQColorSpaceBGRA:
  case kNQColorSpaceCMYK:
    return width * 4;
  default:
    return 0;
  }
}

void NQRgbToGrayLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  for (uint32_t n = 0; n < width; n++) {
    out[n] = NQRgbToGray(in[0], in[1], in[2]);
    in += 3;
  }
}

void NQRgbaToGrayLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  for (uint32_t n = 0; n < width; n++) {
    out[n] = NQRgbToGray(in[0], in[1], in[2]);
    in += 4;
  }
}

void NQBgraToGrayLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  for (uint32_t n = 0; n < width; n++) {
    out[n] = NQRgbToGray(in[2], in[1], in[0]);
    in += 4;
  }
}

void NQBgrxToGrayLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  NQBgraToGrayLine(width, in, out);
}

static void grayToXgxLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  for (uint32_t n = 0; n < width; n++) {
    out[0] = out[1] = out[2] = in[n];
    out += 3;
  }
}

void NQGrayToRgbLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  grayToXgxLine(width, in, out);
}

void NQGrayToBgrLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  grayToXgxLine(width, in, out);
}

void NQGrayToMonochromeLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  for (uint32_t n = 0; n < width; n++) {
    if (in[n] < 128)
      out[n / 8] |= (1 << (7 - (n % 8)));
    else
      out[n / 8] &= ~(1 << (7 - (n % 8)));
  }
}

static void xgxaToXgxLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  for (; width != 0; width--) {
    *out++ = in[0];
    *out++ = in[1];
    *out++ = in[2];
    in += 4;
  }
}

void NQRgbaToRgbLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  xgxaToXgxLine(width, in, out);
}

void NQBgraToBgrLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  xgxaToXgxLine(width, in, out);
}

void NQRgbxToRgbLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  xgxaToXgxLine(width, in, out);
}

void NQBgrxToBgrLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  xgxaToXgxLine(width, in, out);
}

static void reverseXgxaToXgxLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  for (; width != 0; width--) {
    *out++ = in[2];
    *out++ = in[1];
    *out++ = in[0];
    in += 4;
  }
}

void NQRgbaToBgrLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  reverseXgxaToXgxLine(width, in, out);
}

void NQBgraToRgbLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  reverseXgxaToXgxLine(width, in, out);
}

void NQRgbxToBgrLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  reverseXgxaToXgxLine(width, in, out);
}

void NQBgrxToRgbLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  reverseXgxaToXgxLine(width, in, out);
}

static void reverseXgxToXgxLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  for (; width != 0; width--) {
    *out++ = in[2];
    *out++ = in[1];
    *out++ = in[0];
    in += 3;
  }
}

void NQRgbToBgrLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  reverseXgxToXgxLine(width, in, out);
}

void NQBgrToRgbLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  reverseXgxToXgxLine(width, in, out);
}

static void xgxToXgxaLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  for (; width != 0; width--) {
    out[0] = *in++;
    out[1] = *in++;
    out[2] = *in++;
    out += 4;
  }
}

void NQMonochromeToGrayLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  for (uint32_t n = 0; n < width; n++) {
    out[n] = (in[n / 8] & (1 << (7 - (n % 8)))) ? 0x00 : 0xff;
  }
}

void NQCmykToRgbLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  for (; width != 0; width--) {
    NQCmykToRgb(in[0], in[1], in[2], in[3], &out[0], &out[1], &out[2]);
    in += 4;
    out += 3;
  }
}

void NQInvertedCmykToRgbLine(uint32_t width, const uint8_t* in, uint8_t* out)
{
  for (; width != 0; width--) {
    NQInvertedCmykToRgb(in[0], in[1], in[2], in[3], &out[0], &out[1], &out[2]);
    in += 4;
    out += 3;
  }
}
