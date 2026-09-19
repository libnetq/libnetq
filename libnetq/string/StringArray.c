/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/string/StringArray.h"

#include <libnetq/string/String.h>
#include <libnetq/Limits.h>
#include <libnetq/Malloc.h>
#include <libnetq/Assert.h>
#include <libnetq/Log.h>

static NQStringArray8 s_empty8 = {
  .length = 0,
  .characters = { '\0' },
};

NQStringArray8* NQStringArray8_alloc(size_t length)
{
  if (length == 0) {
    return &s_empty8;
  }

  if (length >= NQ_UINT8_MAX) {
    NQ_LOGE("String length exceeded");
    return NULL;
  }

  NQStringArray8* thiz = (NQStringArray8*)NQMalloc(sizeof(*thiz) + length);
  thiz->characters[length] = '\0';
  thiz->length = (uint8_t)length;
  return thiz;
}

NQStringArray8* NQStringArray8_create(const char* characters)
{
  return NQStringArray8_create2(characters, NQStrlen(characters));
}

NQStringArray8* NQStringArray8_create2(const char* characters, size_t length)
{
  NQStringArray8* thiz = NQStringArray8_alloc(length);
  if (thiz != NULL && length != 0)
    memcpy(thiz->characters, characters, length);
  return thiz;
}

void NQStringArray8_destroy(NQStringArray8* thiz)
{
  if (thiz != &s_empty8)
    NQFree((void*)thiz);
}

void NQStringArray8_shrink(NQStringArray8* thiz, size_t newLength)
{
  NQ_ASSERT(newLength <= thiz->length);
  if (newLength < thiz->length) {
    thiz->characters[newLength] = '\0';
    thiz->length = (uint8_t)newLength;
  }
}

static NQStringArray16 s_empty16 = {
  .length = 0,
  .characters = { '\0' },
};

NQStringArray16* NQStringArray16_alloc(size_t length)
{
  if (length == 0) {
    return &s_empty16;
  }

  if (length >= NQ_UINT16_MAX) {
    NQ_LOGE("String length exceeded");
    return NULL;
  }

  NQStringArray16* thiz = (NQStringArray16*)NQMalloc(sizeof(*thiz) + length);
  thiz->characters[length] = '\0';
  thiz->length = (uint16_t)length;
  return thiz;
}

NQStringArray16* NQStringArray16_create(const char* characters)
{
  return NQStringArray16_create2(characters, NQStrlen(characters));
}

NQStringArray16* NQStringArray16_create2(const char* characters, size_t length)
{
  NQStringArray16* thiz = NQStringArray16_alloc(length);
  if (thiz != NULL && length != 0)
    memcpy(thiz->characters, characters, length);
  return thiz;
}

void NQStringArray16_destroy(NQStringArray16* thiz)
{
  if (thiz != &s_empty16)
    NQFree((void*)thiz);
}

void NQStringArray16_shrink(NQStringArray16* thiz, size_t newLength)
{
  NQ_ASSERT(newLength <= thiz->length);
  if (newLength < thiz->length) {
    thiz->characters[newLength] = '\0';
    thiz->length = (uint16_t)newLength;
  }
}

static NQStringArray s_empty = {
  .length = 0,
  .characters = { '\0' },
};

NQStringArray* NQStringArray_alloc(size_t length)
{
  if (length == 0) {
    return &s_empty;
  }

  if (length >= NQ_UINT32_MAX) {
    NQ_LOGE("String length exceeded");
    return NULL;
  }

  NQStringArray* thiz = (NQStringArray*)NQMalloc(sizeof(*thiz) + length);
  thiz->characters[length] = '\0';
  thiz->length = (uint32_t)length;
  return thiz;
}

NQStringArray* NQStringArray_create(const char* characters)
{
  return NQStringArray_create2(characters, NQStrlen(characters));
}

NQStringArray* NQStringArray_create2(const char* characters, size_t length)
{
  NQStringArray* thiz = NQStringArray_alloc(length);
  if (thiz != NULL && length != 0)
    memcpy(thiz->characters, characters, length);
  return thiz;
}

void NQStringArray_destroy(NQStringArray* thiz)
{
  if (thiz != &s_empty)
    NQFree((void*)thiz);
}

void NQStringArray_shrink(NQStringArray* thiz, size_t newLength)
{
  NQ_ASSERT(newLength <= thiz->length);
  if (newLength < thiz->length) {
    thiz->characters[newLength] = '\0';
    thiz->length = (uint32_t)newLength;
  }
}
