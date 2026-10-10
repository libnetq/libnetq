/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/io/ByteBufferChannel.h"

#include <libnetq/string/String.h>
#include <libnetq/MinMax.h>

size_t NQByteBufferChannel_read(NQByteBufferChannel* thiz, void* data, size_t size)
{
  size_t sz = NQByteBuffer_size(&thiz->buffer) - thiz->position;
  size = NQGetMin(sz, size);
  if (size != 0 && data != NULL)
    NQMemcpy(data, NQByteBuffer_data(&thiz->buffer) + thiz->position, size);
  thiz->position += size;
  return size;
}

size_t NQByteBufferChannel_write(NQByteBufferChannel* thiz, const void* data, size_t size)
{
  if (thiz->position < NQByteBuffer_size(&thiz->buffer)) {
    if ((thiz->position + size) > NQByteBuffer_size(&thiz->buffer)) {
      if (!NQByteBuffer_resize(&thiz->buffer, thiz->position + size))
        return 0;
    }
    uint8_t* ptr = NQByteBuffer_data(&thiz->buffer) + thiz->position;
    if (data == NULL)
      NQMemset(ptr, 0, size);
    else
      NQMemcpy(ptr, data, size);
  }
  else {
    if (data == NULL) {
      if ((thiz->position + size) > NQByteBuffer_size(&thiz->buffer)) {
        if (!NQByteBuffer_resize(&thiz->buffer, thiz->position + size))
          return 0;
      }
      uint8_t* ptr = NQByteBuffer_data(&thiz->buffer) + thiz->position;
      NQMemset(ptr, 0, size);
    }
    else if (!NQByteBuffer_insert(&thiz->buffer, thiz->position, data, size))
      return 0;
  }

  thiz->position += size;
  return size;
}
