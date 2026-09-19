/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_BASE64_H
#define _LIBNETQ_BASE64_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_BASE64_NONPAD (1 << 2)
#define NQ_BASE64_URL    (1 << 3)

static inline size_t NQBase64EncodeLength(size_t size)
{
  return ((size + 2) / 3) * 4;
}

NQ_EXPORT int NQBase64EncodeEx(const void* inData, size_t inSize, char* outData, size_t outSize, int flags);
NQ_EXPORT int NQBase64DecodeEx(const char* inData, size_t inSize, void* outData, size_t outSize, int flags);

static inline int NQBase64Encode(const void* inData, size_t inSize, char* outData, size_t outSize)
{
  return NQBase64EncodeEx(inData, inSize, outData, outSize, 0);
}

static inline int NQBase64Decode(const char* inData, size_t inSize, void* outData, size_t outSize)
{
  return NQBase64DecodeEx(inData, inSize, outData, outSize, 0);
}

static inline int NQBase64URLEncode(const void* inData, size_t inSize, char* outData, size_t outSize)
{
  return NQBase64EncodeEx(inData, inSize, outData, outSize, NQ_BASE64_URL);
}

static inline int NQBase64URLDecode(const char* inData, size_t inSize, void* outData, size_t outSize)
{
  return NQBase64DecodeEx(inData, inSize, outData, outSize, NQ_BASE64_URL);
}

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_BASE64_H */
