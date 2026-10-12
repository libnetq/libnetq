/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_CLASS_NAME "NQURL"
#define NQ_LOG_TAG NQ_CLASS_NAME

#include "config.h"
#include "libnetq/URL.h"

#include <libnetq/string/StringUtil.h>
#include <libnetq/Malloc.h>
#include <libnetq/Limits.h>
#include <libnetq/MinMax.h>
#include <libnetq/CType.h>
#include <libnetq/Assert.h>
#include <libnetq/Log.h>

struct NQURLData {
  unsigned schemePosition;
  unsigned schemeLength;
  unsigned hostPosition;
  unsigned hostLength;
  unsigned portPosition;
  unsigned portLength;
  unsigned pathPosition;
  unsigned pathLength;
  unsigned queryPosition;
  unsigned queryLength;
  unsigned fragmentPosition;
  unsigned fragmentLength;
};

struct NQURL {
  struct NQURLData data;
  unsigned length;
  char characters[1];
};


#define NQ_ASCII_DEF(dec, hex, ch, type, ...) kNQURLChar##type,
const unsigned char s_urlCharTypes[] =
{
  #include "libnetq/ASCII.def"
};
#undef NQ_ASCII_DEF

enum NQURLCharType NQGetURLCharType(char ch)
{
  return NQIsASCII(ch) ? s_urlCharTypes[(unsigned)ch] : kNQURLCharOther;
}

static const char lowerDigits[17] = "0123456789abcdef";

static NQ_ALWAYS_INLINE
void checkBuiltinOutput(char* output, size_t outlen)
{
#if NQ_HAS_BUILTIN(__builtin_constant_p)
  if (__builtin_constant_p(output)) {
    NQ_ASSERT(!output && !outlen);
  }
#endif
}

static NQ_ALWAYS_INLINE
int urlEncodeImpl(const char* input, size_t inlen, char* output, size_t outlen, bool pluseAsSpace)
{
  checkBuiltinOutput(output, outlen);

  size_t result = 0;
  for (size_t i = 0; i < inlen; i++) {
    char ch = input[i];
    if (pluseAsSpace && ch == ' ') {
      if (output && result < outlen)
        output[result] = '+';
    }
    else if (NQIsURLCharUnreserved(ch)) {
      if (output && result < outlen)
        output[result] = ch;
    }
    else {
      if (output && result < outlen)
        output[result] = '%';
      result++;

      if (output && result < outlen)
        output[result] = lowerDigits[(ch >> 4) & 0x0f];
      result++;

      if (output && result < outlen)
        output[result] = lowerDigits[ch & 0x0f];
    }
    result++;
  }

  if (output && result < outlen) {
    output[result] = '\0';
  }

  return (int)result;
}

static NQ_ALWAYS_INLINE
int urlDecodeImpl(const char* input, size_t inlen, char* output, size_t outlen, bool pluseAsSpace)
{
  checkBuiltinOutput(output, outlen);

  size_t result = 0;
  for (size_t i = 0; i < inlen; i++) {
    char ch = input[i];
    if (pluseAsSpace && ch == '+') {
      if (output && result < outlen)
        output[result] = ' ';
    }
    else if (ch != '%') {
      if (NQIsURLCharOther(ch))
        return -1;
      if (output && result < outlen)
        output[result] = ch;
    }
    else if (inlen < (i + 3)) {
      if (output && result < outlen)
        output[result] = ch;
    }
    else {
      char hi = input[i + 1];
      if (!NQIsHexDigit(hi)) {
        if (output && result < outlen)
          output[result] = ch;
      }
      else {
        char lo = input[i + 2];
        if (!NQIsHexDigit(lo)) {
          if (output && result < outlen)
            output[result] = ch;
        }
        else {
          if (output && result < outlen)
            output[result] = NQToHexValue(hi) << 4 | NQToHexValue(lo);
          i += 2;
        }
      }
    }
    result++;
  }

  if (output && result < outlen) {
    output[result] = '\0';
  }

  return (int)result;
}

static NQ_ALWAYS_INLINE
int urlEncode(const char* input, size_t inlen, char* output, size_t outlen, bool pluseAsSpace)
{
  if (NQ_INT32_MAX < inlen) {
    NQ_LOGE("URL encode input length (%llu) too larged", (unsigned long long)inlen);
    return -1;
  }
  else if (output) {
    return urlEncodeImpl(input, inlen, output, outlen, pluseAsSpace);
  }
  else {
    return urlEncodeImpl(input, inlen, NULL, 0, pluseAsSpace);
  }
}

static NQ_ALWAYS_INLINE
int urlDecode(const char* input, size_t inlen, char* output, size_t outlen, bool pluseAsSpace)
{
  if (NQ_INT32_MAX < inlen) {
    NQ_LOGE("URL decode input length (%llu) too large", (unsigned long long)inlen);
    return -1;
  }
  else if (output) {
    return urlDecodeImpl(input, inlen, output, outlen, pluseAsSpace);
  }
  else {
    return urlDecodeImpl(input, inlen, NULL, 0, pluseAsSpace);
  }
}

int NQURLEncode(const char* input, size_t inlen, char* output, size_t outlen)
{
  return urlEncode(input, inlen, output, outlen, false);
}

int NQURLDecode(const char* input, size_t inlen, char* output, size_t outlen)
{
  return urlDecode(input, inlen, output, outlen, false);
}

int NQFormURLEncode(const char* input, size_t inlen, char* output, size_t outlen)
{
  return urlEncode(input, inlen, output, outlen, true);
}

int NQFormURLDecode(const char* input, size_t inlen, char* output, size_t outlen)
{
  return urlDecode(input, inlen, output, outlen, true);
}

#define SCHEME_INDEX 0
#define HOST_INDEX 1
#define PORT_INDEX 2
#define PATH_INDEX 3
#define QUERY_INDEX 4
#define FRAGMENT_INDEX 5

static bool NQURLDataParse(const char* characters, size_t length, struct NQURLData* result)
{
  unsigned positions[6];
  unsigned lengths[6];
  char delimiters[6] = { 0, 0, ':', '/', '?', '#' };

  const char* start = NQStrstr(characters, NQ_URL_SCHEME_SEPARATOR);
  if (start == NULL || start == characters) {
    NQ_LOGE("URL cannot be missing scheme");
    return false;
  }

  positions[SCHEME_INDEX] = 0;
  lengths[SCHEME_INDEX] = (unsigned)(start - characters);

  start = start + 3;

  positions[HOST_INDEX] = (unsigned)(start - characters);

  size_t curr = HOST_INDEX;
  for (size_t i = curr + 1; i < 6; i++) {
    const char* end = NQStrchr(start, delimiters[i]);
    if (end == NULL) {
      positions[i] = 0;
      lengths[i] = 0;
      continue;
    }
    lengths[curr] = (unsigned)(end - start);
    start = end;
    positions[i] = (unsigned)(start - characters);
    curr = i;
  }

  lengths[curr] = length - (start - characters);
  if (lengths[HOST_INDEX] == 0) {
    NQ_LOGE("URL cannot be missing host");
    return false;
  }
  if (lengths[PORT_INDEX]) {
    if (lengths[PORT_INDEX] == 1) {
      NQ_LOGE("URL cannot be missing host");
      return false;
    }
    positions[PORT_INDEX]++;
    lengths[PORT_INDEX]--;
  }

  result->schemePosition = positions[SCHEME_INDEX];
  result->schemeLength = lengths[SCHEME_INDEX];
  result->hostPosition = positions[HOST_INDEX];
  result->hostLength = lengths[HOST_INDEX];
  result->portPosition = positions[PORT_INDEX];
  result->portLength = lengths[PORT_INDEX];
  result->pathPosition = positions[PATH_INDEX];
  result->pathLength = lengths[PATH_INDEX];
  result->queryPosition = positions[QUERY_INDEX];
  result->queryLength = lengths[QUERY_INDEX];
  result->fragmentPosition = positions[FRAGMENT_INDEX];
  result->fragmentLength = lengths[FRAGMENT_INDEX];

  return true;
}

NQURL* NQURL_create(const char* characters)
{
  size_t length = NQStrlen(characters);
  if (length > NQ_UINT32_MAX) {
    NQ_LOGE("The URL string is too long");
    return NULL;
  }

  struct NQURLData data;
  if (!NQURLDataParse(characters, length, &data)) {
    return NULL;
  }

  NQURL* thiz = (NQURL*)NQMalloc(sizeof(NQURL) + length);
  if (thiz == NULL) {
    NQ_LOGE("No memory");
    return NULL;
  }

  thiz->data = data;
  thiz->length = (unsigned)length;
  NQMemcpy(thiz->characters, characters, length + 1);

  return thiz;
}

void NQURL_destroy(NQURL* thiz)
{
  NQFree(thiz);
}

const char* NQURL_characters(const NQURL* thiz)
{
  return thiz->characters;
}

size_t NQURL_lenght(NQURL* thiz)
{
  return thiz->length;
}

bool NQURL_isEmpty(const NQURL* thiz)
{
  return thiz->length == 0;
}

static inline size_t getComponent(const NQURL* thiz, unsigned position, unsigned length, char* buf, size_t len)
{
  size_t n = NQGetMin(length, len);
  NQMemcpy(buf, thiz->characters + position, n);
  if (n < len)
    buf[n] = '\0';
  return length;
}

size_t NQURL_getScheme(const NQURL* thiz, char* buffer, size_t length)
{
  return getComponent(thiz, thiz->data.schemePosition, thiz->data.schemeLength, buffer, length);
}

size_t NQURL_getHost(const NQURL* thiz, char* buffer, size_t length)
{
  return getComponent(thiz, thiz->data.hostPosition, thiz->data.hostLength, buffer, length);
}

size_t NQURL_getPort(const NQURL* thiz, char* buffer, size_t length)
{
  return getComponent(thiz, thiz->data.portPosition, thiz->data.portLength, buffer, length);
}

size_t NQURL_getPath(const NQURL* thiz, char* buffer, size_t length)
{
  return getComponent(thiz, thiz->data.pathPosition, thiz->data.pathLength, buffer, length);
}

size_t NQURL_getQuery(const NQURL* thiz, char* buffer, size_t length)
{
  return getComponent(thiz, thiz->data.queryPosition, thiz->data.queryLength, buffer, length);
}

size_t NQURL_getFragment(const NQURL* thiz, char* buffer, size_t length)
{
  return getComponent(thiz, thiz->data.fragmentPosition, thiz->data.fragmentLength, buffer, length);
}
