/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_IO_DATAREADER_H
#define _LIBNETQ_IO_DATAREADER_H

#include <libnetq/Endian.h>
#include <libnetq/MinMax.h>
#include <libnetq/Limits.h>
#include <libnetq/BufferBuilder.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQDataReader NQDataReader;
struct NQDataReader {
  uint8_t* data;
  size_t size;
  uint8_t* ptr;
  uint8_t* end;
};

static inline void NQDataReader_init(NQDataReader* thiz, const void* data, size_t size)
{
  thiz->data = (uint8_t*)data;
  thiz->size = size;
  thiz->ptr = thiz->data;
  thiz->end = thiz->ptr + size;
}

static inline bool NQDataReader_isEmpty(NQDataReader* thiz)
{
  return thiz->ptr >= thiz->end;
}

static inline size_t NQDataReader_size(const NQDataReader* thiz)
{
  return thiz->size;
}

static inline size_t NQDataReader_availableSize(const NQDataReader* thiz)
{
  return thiz->end - thiz->ptr;
}

static inline void* NQDataReader_currentData(const NQDataReader* thiz)
{
  return thiz->ptr;
}

static inline size_t NQDataReader_skip(NQDataReader* thiz, size_t size)
{
  size = NQGetMin(NQDataReader_availableSize(thiz), size);
  thiz->ptr += size;
  return size;
}

static inline bool NQDataReader_skipAll(NQDataReader* thiz, size_t size)
{
  if (size > NQDataReader_availableSize(thiz))
    return false;
  thiz->ptr += size;
  return true;
}

static inline size_t NQDataReader_position(const NQDataReader* thiz)
{
  return thiz->ptr - thiz->data;
}

static inline bool NQDataReader_seekTo(NQDataReader* thiz, size_t position)
{
  uint8_t* newPtr = thiz->data + position;
  if (thiz->end < newPtr)
    return false;
  thiz->ptr = newPtr;
  return true;
}

static inline int NQDataReader_read(NQDataReader* thiz, void* data, size_t size)
{
  size = NQGetMin(size, NQGetMin(NQDataReader_availableSize(thiz), NQ_INT32_MAX));
  if (data != NULL)
    NQMemcpy(data, thiz->ptr, size);
  thiz->ptr += size;
  return (int)size;
}

static inline bool NQDataReader_readAll(NQDataReader* thiz, void* data, size_t size)
{
  if (size > NQDataReader_availableSize(thiz))
    return false;
  if (data != NULL)
    NQMemcpy(data, thiz->ptr, size);
  thiz->ptr += size;
  return true;
}

static inline bool NQDataReader_readUint8(NQDataReader* thiz, uint8_t* value)
{
  if (thiz->ptr >= thiz->end)
    return false;
  if (value != NULL)
    *value = *thiz->ptr;
  thiz->ptr++;
  return true;
}

static inline bool NQDataReader_readUint32LE(NQDataReader* thiz, uint32_t* value)
{
  uint8_t* newPtr = thiz->ptr + sizeof(*value);
  if (newPtr > thiz->end)
    return false;
  if (value != NULL)
    NQGetUint32LE(thiz->ptr, value);
  thiz->ptr = newPtr;
  return true;
}

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_IO_DATAREADER_H */
