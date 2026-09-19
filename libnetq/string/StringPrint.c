/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/string/StringPrint.h"

#include <libnetq/string/StringUtil.h>
#include <libnetq/string/Sprintf.h>
#include <libnetq/Malloc.h>
#include <libnetq/Limits.h>
#include <libnetq/MinMax.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/Assert.h>

void NQStringPrint_init(NQStringPrint* thiz)
{
  thiz->characters = thiz->buffer;
  thiz->length = 0;
  thiz->capacity = sizeof(thiz->buffer);
  thiz->characters[0] = '\0';
}

void NQStringPrint_finalize(NQStringPrint* thiz)
{
  if (thiz->characters != thiz->buffer)
    NQFree(thiz->characters);
}

static bool reserveCapacity(NQStringPrint* thiz, size_t newCapacity)
{
  NQ_ASSERT(newCapacity > thiz->capacity);
  NQ_ASSERT(newCapacity > sizeof(thiz->buffer));

  newCapacity = newCapacity << 1;
  char* newBuffer = (char*)NQMalloc(newCapacity);
  if (newBuffer == NULL)
    return false;

  memcpy(newBuffer, thiz->characters, thiz->length + 1);
  if (thiz->characters != thiz->buffer)
    NQFree(thiz->characters);

  thiz->characters = newBuffer;
  thiz->capacity = newCapacity;

  return true;
}

int NQStringPrint_printf(NQStringPrint* thiz, const char* format, ...)
{
  va_list list;
  va_start(list, format);
  int ret = NQStringPrint_vprintf(thiz, format, list);
  va_end(list);
  return ret;
}

int NQStringPrint_vprintf(NQStringPrint* thiz, const char* format, va_list list)
{
  NQ_ASSERT(thiz->length < thiz->capacity);

  va_list listCopy;

  va_copy(listCopy, list);
  int n = vsnprintf(thiz->characters + thiz->length, thiz->capacity - thiz->length, format, listCopy);
  va_end(listCopy);

  if (n < 0)
    return n;

  size_t newSize = thiz->length + (size_t)n + 1;
  if (thiz->length > newSize)
    return -NQ_EOVERFLOW;

  if (newSize <= thiz->capacity) {
    thiz->length += n;
    return n;
  }

  if (!reserveCapacity(thiz, newSize))
    return -NQ_ENOMEM;

  int ret = vsnprintf(thiz->characters + thiz->length, thiz->capacity - thiz->length, format, list);
  
  NQ_ASSERT(thiz->length + ret + 1 <= thiz->capacity);

  thiz->length += ret;

  NQ_ASSERT(thiz->length < thiz->capacity);

  return ret;
}

int NQStringPrint_write(NQStringPrint* thiz, const char* characters, size_t length)
{
  if (length == 0)
    return 0;

  length = NQGetMin(NQ_INT32_MAX, length);

  int newSize = thiz->length + (int)length + 1;
  if (newSize < thiz->length)
    return -NQ_EOVERFLOW;

  if (newSize > thiz->capacity && !reserveCapacity(thiz, newSize))
    return -NQ_ENOMEM;

  memcpy(thiz->characters + thiz->length, characters, length);
  thiz->length += length;
  thiz->characters[thiz->length] = '\0';

  return (int)length;
}

bool NQStringPrint_writeAll(NQStringPrint* thiz, const char* characters, size_t length)
{
  if (length == 0)
    return false;

  size_t newSize = thiz->length + length + 1;
  if (newSize < thiz->length)
    return false;

  if (newSize > thiz->capacity && !reserveCapacity(thiz, newSize))
    return false;

  memcpy(thiz->characters + thiz->length, characters, length);
  thiz->length += length;
  thiz->characters[thiz->length] = '\0';

  return true;
}

void NQStringPrint_reset(NQStringPrint* thiz)
{
  thiz->length = 0;
  thiz->characters[0] = '\0';
}

bool NQStringPrint_resize(NQStringPrint* thiz, size_t length)
{
  if (length > NQ_INT32_MAX)
    return false;

  size_t newSize = length + 1;
  if (newSize > thiz->capacity && !reserveCapacity(thiz, newSize))
    return false;

  if (length < thiz->length)
    thiz->characters[length] = '\0';
  else
    memset(thiz->characters + thiz->length + 1, 0, newSize - thiz->length - 1);

  thiz->length = length;
  return true;
}
