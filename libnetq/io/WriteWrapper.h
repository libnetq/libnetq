/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_IO_WRITEWRAPPER_H
#define _LIBNETQ_IO_WRITEWRAPPER_H

#include <libnetq/Endian.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*NQWriteCallback) (void* userdata, const void* data, size_t size);

typedef struct NQWriteWrapper NQWriteWrapper;
struct NQWriteWrapper {
  NQWriteCallback write;
  void* userdata;
  uint64_t numberOfBytesWritten;
};

static inline void NQWriteWrapper_init(NQWriteWrapper* thiz, NQWriteCallback write, void* userdata)
{
  thiz->write = write;
  thiz->userdata = userdata;
  thiz->numberOfBytesWritten = 0;
}

static inline int NQWriteWrapper_write(NQWriteWrapper* thiz, const void* data, size_t size)
{
  int ret = thiz->write(thiz->userdata, data, size);
  if (ret > 0)
    thiz->numberOfBytesWritten += ret;
  return ret;
}

NQ_EXPORT bool NQWriteWrapper_writeAll(NQWriteWrapper*, const void* data, size_t size);

static inline bool NQWriteWrapper_writeUint8(NQWriteWrapper* thiz, uint8_t value)
{
  return NQWriteWrapper_writeAll(thiz, &value, sizeof(value));
}

static inline bool NQWriteWrapper_writeUint16LE(NQWriteWrapper* thiz, uint16_t value)
{
  value = NQHostToLE16(value);
  return NQWriteWrapper_writeAll(thiz, &value, sizeof(value));
}

static inline bool NQWriteWrapper_writeUint16BE(NQWriteWrapper* thiz, uint16_t value)
{
  value = NQHostToBE16(value);
  return NQWriteWrapper_writeAll(thiz, &value, sizeof(value));
}

static inline bool NQWriteWrapper_writeInt32LE(NQWriteWrapper* thiz, int32_t value)
{
  value = NQHostToLE32(value);
  return NQWriteWrapper_writeAll(thiz, &value, sizeof(value));
}

static inline bool NQWriteWrapper_writeInt32BE(NQWriteWrapper* thiz, int32_t value)
{
  value = NQHostToBE32(value);
  return NQWriteWrapper_writeAll(thiz, &value, sizeof(value));
}

static inline bool NQWriteWrapper_writeUint32LE(NQWriteWrapper* thiz, uint32_t value)
{
  value = NQHostToLE32(value);
  return NQWriteWrapper_writeAll(thiz, &value, sizeof(value));
}

static inline bool NQWriteWrapper_writeUint32BE(NQWriteWrapper* thiz, uint32_t value)
{
  value = NQHostToBE32(value);
  return NQWriteWrapper_writeAll(thiz, &value, sizeof(value));
}

static inline bool NQWriteWrapper_writeInt64LE(NQWriteWrapper* thiz, int64_t value)
{
  value = NQHostToLE64(value);
  return NQWriteWrapper_writeAll(thiz, &value, sizeof(value));
}

static inline bool NQWriteWrapper_writeInt64BE(NQWriteWrapper* thiz, int64_t value)
{
  value = NQHostToBE64(value);
  return NQWriteWrapper_writeAll(thiz, &value, sizeof(value));
}

static inline bool NQWriteWrapper_writeUint64LE(NQWriteWrapper* thiz, uint64_t value)
{
  value = NQHostToLE64(value);
  return NQWriteWrapper_writeAll(thiz, &value, sizeof(value));
}

static inline bool NQWriteWrapper_writeUint64BE(NQWriteWrapper* thiz, uint64_t value)
{
  value = NQHostToBE64(value);
  return NQWriteWrapper_writeAll(thiz, &value, sizeof(value));
}

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_IO_WRITEWRAPPER_H */
