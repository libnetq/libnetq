/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/Base64.h"

#include <libnetq/ErrorCode.h>
#include <libnetq/Limits.h>
#include <libnetq/Assert.h>

#define UNKN 0xFF

static const char base64EncMap[64] = {
  0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
  0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
  0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
  0x59, 0x5A, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
  0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E,
  0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
  0x77, 0x78, 0x79, 0x7A, 0x30, 0x31, 0x32, 0x33,
  0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x2B, 0x2F
};

static const uint8_t base64DecMap[128] = {
  UNKN, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN,
  UNKN, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN,
  UNKN, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN,
  UNKN, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN,
  UNKN, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN,
  UNKN, UNKN, UNKN, 0x3E, UNKN, UNKN, UNKN, 0x3F,
  0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,
  0x3C, 0x3D, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN,
  UNKN, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
  0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
  0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
  0x17, 0x18, 0x19, UNKN, UNKN, UNKN, UNKN, UNKN,
  UNKN, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
  0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
  0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,
  0x31, 0x32, 0x33, UNKN, UNKN, UNKN, UNKN, UNKN
};

static const char base64URLEncMap[64] = {
  0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
  0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
  0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
  0x59, 0x5A, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
  0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E,
  0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
  0x77, 0x78, 0x79, 0x7A, 0x30, 0x31, 0x32, 0x33,
  0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x2D, 0x5F
};

static const uint8_t base64URLDecMap[128] = {
  UNKN, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN,
  UNKN, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN,
  UNKN, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN,
  UNKN, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN,
  UNKN, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN,
  UNKN, UNKN, UNKN, UNKN, UNKN, 0x3E, UNKN, UNKN,
  0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,
  0x3C, 0x3D, UNKN, UNKN, UNKN, UNKN, UNKN, UNKN,
  UNKN, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
  0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
  0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
  0x17, 0x18, 0x19, UNKN, UNKN, UNKN, UNKN, 0x3F,
  UNKN, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
  0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
  0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,
  0x31, 0x32, 0x33, UNKN, UNKN, UNKN, UNKN, UNKN
};

int NQBase64EncodeEx(const void* inData, size_t inSize, char* outData, size_t outSize, int flags)
{
  uint8_t b;
  const char* map;

  if (inData == NULL)
    return -NQ_EINVAL;

  size_t result = NQBase64EncodeLength(inSize);
  if (result < inSize || NQ_INT32_MAX < result)
    return -NQ_EINVAL;

  if (flags & NQ_BASE64_NONPAD) {
    size_t rem = inSize % 3;
    if (rem != 0)
      result -= (3 - rem);
  }

  if (outSize == 0) {
    return (int)result;
  }
  if (result == 0) {
    *outData = '\0';
    return 0;
  }

  NQ_ASSERT(outData);

  const uint8_t* inStart = inData;
  const uint8_t* inEnd = (const uint8_t*)inData + inSize;

  char* outStart = outData;
  char* outEnd = outData + outSize;

  if (flags & NQ_BASE64_URL)
    map = base64URLEncMap;
  else
    map = base64EncMap;

  for (;;) {
    b = (*inStart >> 2) & 0x3F;
    *outStart++ = map[b];
    if (outEnd <= outStart)
      break;

    b = (*inStart++ << 4) & 0x3F;
    if (inEnd <= inStart) {
      *outStart++ = map[b];
      if (outEnd <= outStart)
        break;
      if (!(flags & NQ_BASE64_NONPAD)) {
        *outStart++ = '=';
        if (outEnd <= outStart)
          break;
        *outStart++ = '=';
        if (outEnd <= outStart)
          break;
      }
      *outStart++ = '\0';
      break;
    }

    b |= (*inStart >> 4) & 0x0F;
    *outStart++ = map[b];
    if (outEnd <= outStart)
      break;

    b = (*inStart++ << 2) & 0x3F;
    if (inEnd <= inStart) {
      *outStart++ = map[b];
      if (outEnd <= outStart)
        break;
      if (!(flags & NQ_BASE64_NONPAD)) {
        *outStart++ = '=';
        if (outEnd <= outStart)
          break;
      }
      *outStart++ = '\0';
      break;
    }

    b |= (*inStart >> 6) & 0x03;
    *outStart++ = map[b];
    if (outEnd <= outStart)
      break;

    b = *inStart++ & 0x3F;
    *outStart++ = map[b];
    if (outEnd <= outStart)
      break;

    if (inEnd <= inStart) {
      if (outStart < outEnd)
        *outStart = '\0';
      break;
    }
  }

  return result;
}

int NQBase64DecodeEx(const char* inData, size_t inSize, void* outData, size_t outSize, int flags)
{
  uint8_t curr;
  uint8_t b1, b2, b3, b4;

  if (inData == NULL)
    return -NQ_EINVAL;

  if (inSize == 0) {
    if (outSize != 0)
      *(uint8_t*)outData = '\0';
    return 0;
  }

  const uint8_t* inStart = (const uint8_t*)inData;
  const uint8_t* inEnd = inStart + inSize;

  uint8_t* outStart;
  uint8_t* outEnd;

  if (outData) {
    outStart = outData;
    outEnd = (uint8_t*)outData + outSize;
  }
  else {
    outStart = NULL;
    outEnd = NULL;
  }

  int result = 0;
  const uint8_t* map = (flags & NQ_BASE64_URL) ? base64URLDecMap : base64DecMap;
  for (;;) {
    curr = *inStart++;
    b1 = map[curr];
    if (b1 == UNKN) {
      result = -NQ_EINVAL;
      break;
    }

    if (inEnd <= inStart) {
      uint8_t pad = (b1 << 2);
      if (flags & NQ_BASE64_NONPAD && pad == 0)
        break;
      // result++;
      // if (outStart < outEnd)
      //   *outStart++ = pad;
      result = -NQ_EINVAL;
      break;
    }

    curr = *inStart++;
    b2 = map[curr];
    if (b2 == UNKN) {
      result = -NQ_EINVAL;
      break;
    }

    result++;
    if (outStart < outEnd)
      *outStart++ = (b1 << 2) | (b2 >> 4);

    if (inEnd <= inStart) {
      uint8_t pad = ((b2 << 4) & 0xF0);
      if (flags & NQ_BASE64_NONPAD && pad == 0)
        break;
      // result++;
      // if (outStart < outEnd)
      //   *outStart++ = pad;
      result = -NQ_EINVAL;
      break;
    }

    curr = *inStart++;
    b3 = map[curr];
    if (b3 == UNKN) {
      if ((flags & NQ_BASE64_NONPAD) == 0 && curr == '=' && inStart < inEnd && *inStart == '=') {
        if (++inStart == inEnd)
          break;
      }
      result = -NQ_EINVAL;
      break;
    }

    result++;
    if (outStart < outEnd)
      *outStart++ = ((b2 << 4) & 0xF0) | (b3 >> 2);

    if (inEnd <= inStart) {
      uint8_t pad = (b3 << 6);
      if (flags & NQ_BASE64_NONPAD && pad == 0)
        break;
      // result++;
      // if (outStart < outEnd)
      //   *outStart++ = pad;
      result = -NQ_EINVAL;
      break;
    }

    curr = *inStart++;
    b4 = map[curr];
    if (b4 == UNKN) {
      if ((flags & NQ_BASE64_NONPAD) == 0 && curr == '=' && inStart == inEnd)
        break;
      result = -NQ_EINVAL;
      break;
    }

    result++;
    if (outStart < outEnd)
      *outStart++ = ((b3 << 6) | b4);

    if (inEnd <= inStart)
      break;
  }

  if (outStart < outEnd)
    *outStart = '\0';

  return result;
}
