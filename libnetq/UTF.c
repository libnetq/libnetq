/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/UTF.h"

#ifdef NQ_OS_WINDOWS
#include <windows.h>
#endif

#include <libnetq/Malloc.h>
#include <libnetq/Limits.h>
#include <libnetq/string/StringUtil.h>
#include <libnetq/Assert.h>

// UTF-8
// U+00000000  U+0000007F [0x00000080] 0xxxxxxx
// U+00000080  U+000007FF [0x00000780] 110xxxxx 10xxxxxx
// U+00000800  U+0000FFFF [0x0000F800] 1110xxxx 10xxxxxx 10xxxxxx
// U+00010000  U+001FFFFF [0x001F0000] 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
// U+00200000  U+03FFFFFF [0x03E00000] 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
// U+04000000  U+7FFFFFFF [0x7C000000] 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx

// U+00000000  U+0000007F: 00 - 7F
// U+00000080  U+000007FF: C2 80 - DF BF
// U+00000800  U+0000FFFF: E0 A0 80 - EF BF BF
// U+00010000  U+001FFFFF: F0 90 80 80 - F7 BF BF BF
// U+00200000  U+03FFFFFF: F8 90 80 80 80 - FB BF BF BF BF
// U+04000000  U+7FFFFFFF: FC 84 80 80 80 80 - FD BF BF BF BF BF

// UTF-16
// U+D800  U+DBFF [high surrogates] 110110yyyyyyyyyy
// U+DC00  U+DFFF [low  surrogates] 110111xxxxxxxxxx

// U' = yyyyyyyyyyxxxxxxxxxx  // U - 0x10000
// W1 = 110110yyyyyyyyyy      // 0xD800 + yyyyyyyyyy
// W2 = 110111xxxxxxxxxx      // 0xDC00 + xxxxxxxxxx

// U+00000000  U+0000D7FF: xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
// U+0000E000  U+0000FFFF: xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
// U+00010000  U+0010FFFF: 110110xxxxxxxxxx 110111xxxxxxxxxx

#if defined(NQ_OS_WINDOWS) && NQ_CHECK_LEVEL >= 2
static bool UTF8ToUTF16Check(const uint8_t* utf8Start, const uint8_t* utf8End, uint16_t* utf16Start, uint16_t* utf16End)
{
  int u8Size = (int)(utf8End - utf8Start);
  int length = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)utf8Start, u8Size, NULL, 0);

  if (length < 0 && (size_t)length != utf16End - utf16Start)
    return false;

  WCHAR* data = (WCHAR*)NQMalloc(length * sizeof(WCHAR));
  MultiByteToWideChar(CP_UTF8, 0, (LPCCH)utf8Start, u8Size, data, length);

  bool result = (memcmp(utf16Start, data, length) == 0);

  NQFree(data);

  return result;
}
#else
#define UTF8ToUTF16Check(...) true
#endif

static inline size_t npf_uchar8_size(uint8_t b1)
{  
  if (b1 < 0x80)
    return 1;

  if (b1 < 0xC0)
    return 0;

  if (b1 < 0xE0)
    return 2;

  if (b1 < 0xF0)
    return 3;

  if (b1 < 0xF8)
    return 4;

  if (b1 < 0xFC)
    return 5; // optional
  
  if (b1 < 0xFE)
    return 6; // optional

  return 0;
}

size_t NQUCharSize8(NQUChar character)
{
  if (character <= 0x7F)
    return 1;
  if (character <= 0x7FF)
    return 2;
  if (character <= 0xFFFF)
    return 3;
  if (character <= 0x1FFFFF)
    return 4;
  if (character <= 0x3FFFFFF)
    return 5; // optional
  if (character <= 0x7FFFFFFF)
    return 6; // optional
  return 0;
}

NQUChar NQUCharNext8(const uint8_t* start, const uint8_t* end, const uint8_t** charEnd)
{
  NQUChar value;
  uint8_t b;
  size_t index, size;
  static const uint8_t mask1[] = { 0x00, 0x00, 0x1F, 0x0F, 0x07, 0x03, 0x01 };

  if (end <= start)
    return NQNonUChar;

  b = *start++;
  size = npf_uchar8_size(b);

  if (size == 0)
    return NQNonUChar;
  else if (size == 1)
    value = b;
  else {
    if (end < start + size - 1)
      return NQNonUChar;

    value = b & mask1[size];
    for (index = 1; index < size; index++) {
      b = *start++;
      if (b < 0x80 || 0xC0 <= b)
        return NQNonUChar;
      value = (value << 6) | (b & 0x3F);
    }
  }

  if (charEnd)
    *charEnd = start;

  return value;
}

size_t NQUCharPush8(uint8_t* start, uint8_t* end, uint8_t** charEnd, NQUChar character)
{
  if (character < 0)
    return 0;

  if (end == NULL)
    end = start + 6;

  if (end <= start)
    return 0;

  size_t n = 0;
  if (character < 0x80)
    start[n++] = (uint8_t)character;
  else if (character < 0x800) {
    if (end <= start + 1)
      return 0;
    start[n++] = ((character >>  6) | 0xC0);
    start[n++] = ((character >>  0) & 0x3F) | 0x80;
  }
  else if (character < 0x10000) {
    if (end <= start + 2)
      return 0;
    start[n++] = ((character >> 12) | 0xE0);
    start[n++] = ((character >>  6) & 0x3F) | 0x80;
    start[n++] = ((character >>  0) & 0x3F) | 0x80;
  }
  else if (character < 0x200000) {
    if (end <= start + 3)
      return 0;
    start[n++] = ((character >> 18) | 0xF0);
    start[n++] = ((character >> 12) & 0x3F) | 0x80;
    start[n++] = ((character >>  6) & 0x3F) | 0x80;
    start[n++] = ((character >>  0) & 0x3F) | 0x80;
  }
  else if (character < 0x4000000) {
    if (end <= start + 4)
      return 0;
    start[n++] = ((character >> 24) | 0xF8);
    start[n++] = ((character >> 18) & 0x3F) | 0x80;
    start[n++] = ((character >> 12) & 0x3F) | 0x80;
    start[n++] = ((character >>  6) & 0x3F) | 0x80;
    start[n++] = ((character >>  0) & 0x3F) | 0x80;
  }
  else {
    if (end <= start + 5)
      return 0;
    start[n++] = ((character >> 30) | 0xFC);
    start[n++] = ((character >> 24) & 0x3F) | 0x80;
    start[n++] = ((character >> 18) & 0x3F) | 0x80;
    start[n++] = ((character >> 12) & 0x3F) | 0x80;
    start[n++] = ((character >>  6) & 0x3F) | 0x80;
    start[n++] = ((character >>  0) & 0x3F) | 0x80;
  }

  if (charEnd)
    *charEnd = start + n;

  return n;
}

static inline bool isHighSurrogates16(uint16_t c)
{
  return (c & 0xFC00) == 0xD800;
}

static inline bool isLowSurrogates16(uint16_t c)
{
  return (c & 0xFC00) == 0xDC00;
}

static inline size_t UTF16CharSize(uint16_t c)
{
  return isHighSurrogates16(c) ? 2 : 1;
}

NQUChar NQUCharNext16(const uint16_t* start, const uint16_t* end, const uint16_t** charEnd)
{
  NQUChar result;
  uint16_t hightChar, lowChar;

  if (end == NULL)
    end = start + 2;

  if (end <= start)
    return NQNonUChar;

  hightChar = *start++;
  if (isHighSurrogates16(hightChar)) {
    if (end <= start)
      return NQNonUChar;
    lowChar = *start++;
    if (!isLowSurrogates16(lowChar))
      return NQNonUChar;
    result = ((hightChar & 0xFC00) << 10) | (lowChar & 0xFC00);
  }
  else if (isLowSurrogates16(hightChar))
    return NQNonUChar;
  else
    result = hightChar;

  if (charEnd)
    *charEnd = start;

  return result;
}

size_t NQUCharSize16(NQUChar character)
{
  if (character < 0)
    return 0;
  
  if (character < 0x10000)
    return 1;

  if (character < 0x110000)
    return 2;
  
  return 0;
}

size_t NQUCharPush16(uint16_t* start, uint16_t* end, uint16_t** charEnd, NQUChar character)
{
  if (character < 0)
    return 0;

  if (end != NULL && end <= start)
    return 0;

  size_t n = 0;
  if (character < 0x10000)
    start[n++] = (uint16_t)character;
  else if (character < 0x110000) {
    if (end != NULL && end <= start + 1)
      return 0;

    start[n++] = (uint16_t)((character >> 10) & 0x3FF);
    start[n++] = (uint16_t)(character & 0x3FF);
  }
  else
    return 0;

  if (charEnd)
    *charEnd = start + n;

  return n;
}

bool NQCalculateUTF8Info(const uint8_t* utf8Start, const uint8_t* utf8End, NQUnicodeInfo* info)
{
  if (utf8Start == NULL)
    return false;

  if (utf8End == NULL)
    utf8End = utf8Start + NQStrlen((const char*)utf8Start);

  NQUChar character;
  NQUChar orAllChars = 0;
  size_t n, u16Size = 0, u32Size = 0;
  const uint8_t* utf8Ptr = utf8Start;
  bool result = true;
  
  while (utf8Ptr < utf8End) {
    character = NQUCharNext8(utf8Ptr, utf8End, &utf8Ptr);
    if (character < 0) {
      result = false;
      break;
    }
    n = NQUCharSize16(character);
    if (n == 0) {
      result = false;
      break;
    }
    u16Size += n;
    u32Size++;
    orAllChars |= character;
  }

  if (info) {
    info->utf8Size = (utf8Ptr - utf8Start);
    info->utf16Size = u16Size;
    info->utf32Size = u32Size;
    info->isAllASCII = (orAllChars < 0x80);
  }

  return result;
}

bool NQConvertUTF8ToUTF16(const uint8_t* utf8Start, const uint8_t* utf8End, uint16_t* utf16Start, uint16_t* utf16End, NQUnicodeInfo* info)
{
  if (utf8Start == NULL)
    return false;

  if (utf8End == NULL)
    utf8End = utf8Start + NQStrlen((const char*)utf8Start);

  if (utf16Start == NULL)
    return NQCalculateUTF8Info(utf8Start, utf8End, info);

  NQUChar character;
  NQUChar orAllChars = 0;
  size_t count = 0;
  const uint8_t* utf8Ptr = utf8Start;
  uint16_t* utf16Ptr = utf16Start;
  bool result = true;

  while (utf8Ptr < utf8End) {
    character = NQUCharNext8(utf8Ptr, utf8End, &utf8Ptr);
    if (character < 0) {
      result = false;
      break;
    }

    if (result && !NQUCharPush16(utf16Ptr, utf16End, &utf16Ptr, character))
      result = false;

    count++;
    orAllChars |= character;
  }

  if (utf16Ptr < utf16End || utf16End == NULL)
    *utf16Ptr = '\0';

  if (info) {
    info->utf8Size = (utf8Ptr - utf8Start);
    info->utf16Size = (utf16Ptr - utf16Start);
    info->utf32Size = count;
    info->isAllASCII = (orAllChars < 0x80);
  }

  NQ_ASSERT(UTF8ToUTF16Check(utf8Start, utf8End, utf16Start, utf16Ptr));
  return result;
}

bool NQCalculateUTF16Info(const uint16_t* utf16Start, const uint16_t* utf16End, NQUnicodeInfo* info)
{
  if (utf16Start == NULL)
    return false;

  if (utf16End == NULL)
    utf16End = utf16Start + NQCStrLen16(utf16Start);

  NQUChar character;
  int32_t orAllChars = 0;
  size_t n, u8Size = 0, u32Size = 0;
  const uint16_t* utf16Ptr = utf16Start;
  bool result = true;

  while (utf16Ptr < utf16End) {
    character = NQUCharNext16(utf16Ptr, utf16End, &utf16Ptr);
    if (character < 0) {
      result = false;
      break;
    }
    n = NQUCharSize8(character);
    if (n == 0) {
      result = false;
      break;
    }
    u8Size += n;
    u32Size++;
    orAllChars |= character;
  }

  if (info) {
    info->utf8Size = u8Size;
    info->utf16Size = (utf16Ptr - utf16Start);
    info->utf32Size = u32Size;
    info->isAllASCII = (orAllChars < 0x80);
  }

  return result;
}

bool NQConvertUTF16ToUTF8(const uint16_t* utf16Start, const uint16_t* utf16End, uint8_t* utf8Start, uint8_t* utf8End, NQUnicodeInfo* info)
{
  if (utf16Start == NULL)
    return false;

  if (utf16End == NULL)
    utf16End = utf16Start + NQCStrLen16(utf16Start);

  if (utf8Start == NULL)
    return NQCalculateUTF16Info(utf16Start, utf16End, info);

  NQUChar character;
  int32_t orAllChars = 0;
  size_t count = 0;
  const uint16_t* utf16Ptr = utf16Start;
  uint8_t* utf8Ptr = utf8Start;
  bool result = true;

  while (utf16Ptr < utf16End) {
    character = NQUCharNext16(utf16Ptr, utf16End, &utf16Ptr);
    if (character < 0) {
      result = false;
      break;
    }

    if (result && !NQUCharPush8(utf8Ptr, utf8End, &utf8Ptr, character))
      result = false;

    count++;
    orAllChars |= character;
  }

  if (utf8Ptr < utf8End || utf8End == NULL)
    *utf8Ptr = '\0';

  if (info) {
    info->utf8Size = (utf8Ptr - utf8Start);
    info->utf16Size = (utf16Ptr - utf16Start);
    info->utf32Size = count;
    info->isAllASCII = (orAllChars < 0x80);
  }

  return result;
}
