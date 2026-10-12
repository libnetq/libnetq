/*
 * MIT License
 *
 * Copyright (c) 2022-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/ByteBuffer.h"

#include <libnetq/string/StringUtil.h>
#include <libnetq/Malloc.h>
#include <libnetq/MinMax.h>
#include <libnetq/Assert.h>
#include <libnetq/Limits.h>

void NQByteBuffer_init(NQByteBuffer* thiz)
{
  thiz->data = NULL;
  thiz->size = 0;
  thiz->capacity = 0;
}

void NQByteBuffer_finalize(NQByteBuffer* thiz)
{
  NQByteBuffer_clear(thiz);
}

static bool NQByteBuffer_reserveCapacity(NQByteBuffer* thiz, size_t newCapacity)
{
  if (NQ_UINT32_MAX < newCapacity)
    return false;

  if (newCapacity <= thiz->capacity)
    return true;

  uint8_t* oldData = thiz->data;
  uint8_t* newData = (uint8_t*)NQMalloc(newCapacity);
  if (newData == NULL)
    return false;

  if (oldData != NULL) {
    NQMemcpy(newData, oldData, thiz->size);
    NQFree(oldData);
  }

  thiz->data = newData;
  thiz->capacity = (uint32_t)newCapacity;

  return true;
}

static inline size_t NQByteBuffer_nextCapacity(NQByteBuffer* thiz, size_t newMinCapacity)
{
  return NQGetMax(newMinCapacity, thiz->capacity + thiz->capacity / 4 + 1);
}

static bool NQByteBuffer_expandCapacity(NQByteBuffer* thiz, size_t newMinCapacity)
{
  return NQByteBuffer_reserveCapacity(thiz, NQByteBuffer_nextCapacity(thiz, newMinCapacity));
}

void NQByteBuffer_clear(NQByteBuffer* thiz)
{
  if (thiz->data != NULL) {
    NQFree(thiz->data);
    thiz->data = NULL;
  }
  thiz->size = 0;
  thiz->capacity = 0;
}

bool NQByteBuffer_resize(NQByteBuffer* thiz, size_t size)
{
  if (size > thiz->capacity && !NQByteBuffer_expandCapacity(thiz, size))
    return false;

  thiz->size = (uint32_t)size;
  return true;
}

bool NQByteBuffer_insert(NQByteBuffer* thiz, size_t position, const uint8_t* data, size_t size)
{
  size_t newSize;

  if (size == 0)
    return true;

  if (position > thiz->size) {
    newSize = position + size;
    if (newSize < thiz->size)
      return false;
    if (newSize > thiz->capacity) {
      size_t capacity = NQByteBuffer_nextCapacity(thiz, newSize);
      uint8_t* newData = (uint8_t*)NQMalloc(capacity);

      NQMemcpy(newData, thiz->data, thiz->size);

      NQFree(thiz->data);

      thiz->data = newData;
      thiz->capacity = (uint32_t)capacity;
    }
    NQMemset(thiz->data + thiz->size, 0, position - thiz->size);
  }
  else {
    newSize = thiz->size + size;
    if (newSize < thiz->size)
      return false;

    if (newSize <= thiz->capacity) {
      if (position != thiz->size)
        NQMemmove(thiz->data + position + size, thiz->data + position, thiz->size - position);
    }
    else {
      size_t capacity = NQByteBuffer_nextCapacity(thiz, newSize);
      uint8_t* newData = (uint8_t*)NQMalloc(capacity);

      if (position != 0)
        NQMemcpy(newData, thiz->data, position);

      size_t remainder = thiz->size - position;
      if (remainder != 0)
        NQMemcpy(newData + position + size, thiz->data + position, remainder);

      NQFree(thiz->data);

      thiz->data = newData;
      thiz->capacity = (uint32_t)capacity;
    }
  }

  NQMemcpy(thiz->data + position, data, size);
  thiz->size = (uint32_t)newSize;

  return true;
}

bool NQByteBuffer_append(NQByteBuffer* thiz, const uint8_t* data, size_t size)
{
  if (size == 0)
    return true;

  size_t newSize = thiz->size + size;
  if (newSize < thiz->size)
    return false;

  if (newSize > thiz->capacity) {
    if (!NQByteBuffer_expandCapacity(thiz, newSize))
      return false;
  }

  NQMemcpy(thiz->data + thiz->size, data, size);
  thiz->size = newSize;
  return true;
}

void NQByteBuffer_swap(NQByteBuffer* thiz, NQByteBuffer* other)
{
  uint8_t* tmpData;
  uint32_t tmpSize;

  tmpData = thiz->data;
  thiz->data = other->data;
  other->data = tmpData;

  tmpSize = thiz->size;
  thiz->size = other->size;
  other->size = tmpSize;

  tmpSize = thiz->capacity;
  thiz->capacity = other->capacity;
  other->capacity = tmpSize;
}

void NQByteBuffer_removeAt(NQByteBuffer* thiz, size_t position, size_t size)
{
  NQ_ASSERT((position + size) <= thiz->size);
  uint8_t* dst = thiz->data + position;
  const uint8_t* src = dst + size;
  (void)NQMemmove(dst, src, thiz->data + thiz->size - src);
  thiz->size -= size;
}
