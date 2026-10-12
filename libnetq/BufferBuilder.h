/*
 * MIT License
 *
 * Copyright (c) 2021-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_BUFFERBUILDER_H
#define _LIBNETQ_BUFFERBUILDER_H

#include <libnetq/Endian.h>
#include <libnetq/string/String.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline size_t NQGetBytes(const void* buffer, uint8_t* data, size_t size)
{
  NQMemcpy(data, buffer, size);
  return size;
}

static inline size_t NQGetInt8(const void* buffer, int8_t* value)
{
  *value = *((int8_t*)buffer);
  return sizeof(*value);
}

static inline size_t NQGetUint8(const void* buffer, uint8_t* value)
{
  *value = *((uint8_t*)buffer);
  return sizeof(*value);
}

NQ_EXPORT size_t NQGetInt16(const void* buffer, int16_t* value);
NQ_EXPORT size_t NQGetInt16LE(const void* buffer, int16_t* value);
NQ_EXPORT size_t NQGetInt16BE(const void* buffer, int16_t* value);

NQ_EXPORT size_t NQGetUint16(const void* buffer, uint16_t* value);
NQ_EXPORT size_t NQGetUint16LE(const void* buffer, uint16_t* value);
NQ_EXPORT size_t NQGetUint16BE(const void* buffer, uint16_t* value);

NQ_EXPORT size_t NQGetInt24(const void* buffer, int32_t* value);
NQ_EXPORT size_t NQGetInt24LE(const void* buffer, int32_t* value);
NQ_EXPORT size_t NQGetInt24BE(const void* buffer, int32_t* value);

NQ_EXPORT size_t NQGetUint24(const void* buffer, uint32_t* value);
NQ_EXPORT size_t NQGetUint24LE(const void* buffer, uint32_t* value);
NQ_EXPORT size_t NQGetUint24BE(const void* buffer, uint32_t* value);

NQ_EXPORT size_t NQGetInt32(const void* buffer, int32_t* value);
NQ_EXPORT size_t NQGetInt32LE(const void* buffer, int32_t* value);
NQ_EXPORT size_t NQGetInt32BE(const void* buffer, int32_t* value);

NQ_EXPORT size_t NQGetUint32(const void* buffer, uint32_t* value);
NQ_EXPORT size_t NQGetUint32LE(const void* buffer, uint32_t* value);
NQ_EXPORT size_t NQGetUint32BE(const void* buffer, uint32_t* value);

NQ_EXPORT size_t NQGetInt64(const void* buffer, int64_t* value);
NQ_EXPORT size_t NQGetInt64LE(const void* buffer, int64_t* value);
NQ_EXPORT size_t NQGetInt64BE(const void* buffer, int64_t* value);

NQ_EXPORT size_t NQGetUint64(const void* buffer, uint64_t* value);
NQ_EXPORT size_t NQGetUint64LE(const void* buffer, uint64_t* value);
NQ_EXPORT size_t NQGetUint64BE(const void* buffer, uint64_t* value);


NQ_EXPORT size_t NQPutBytes(void* buffer, const uint8_t* data, size_t size);

NQ_EXPORT size_t NQPutInt8(void* buffer, int8_t value);
NQ_EXPORT size_t NQPutUint8(void* buffer, uint8_t value);

NQ_EXPORT size_t NQPutInt16(void* buffer, int16_t value);
NQ_EXPORT size_t NQPutInt16LE(void* buffer, int16_t value);
NQ_EXPORT size_t NQPutInt16BE(void* buffer, int16_t value);

NQ_EXPORT size_t NQPutUint16(void* buffer, uint16_t value);
NQ_EXPORT size_t NQPutUint16LE(void* buffer, uint16_t value);
NQ_EXPORT size_t NQPutUint16BE(void* buffer, uint16_t value);

NQ_EXPORT size_t NQPutInt24(void* buffer, int32_t value);
NQ_EXPORT size_t NQPutInt24LE(void* buffer, int32_t value);
NQ_EXPORT size_t NQPutInt24BE(void* buffer, int32_t value);

NQ_EXPORT size_t NQPutUint24(void* buffer, uint32_t value);
NQ_EXPORT size_t NQPutUint24LE(void* buffer, uint32_t value);
NQ_EXPORT size_t NQPutUint24BE(void* buffer, uint32_t value);

NQ_EXPORT size_t NQPutInt32(void* buffer, int32_t value);
NQ_EXPORT size_t NQPutInt32LE(void* buffer, int32_t value);
NQ_EXPORT size_t NQPutInt32BE(void* buffer, int32_t value);

NQ_EXPORT size_t NQPutUint32(void* buffer, uint32_t value);
NQ_EXPORT size_t NQPutUint32LE(void* buffer, uint32_t value);
NQ_EXPORT size_t NQPutUint32BE(void* buffer, uint32_t value);

NQ_EXPORT size_t NQPutInt64(void* buffer, int64_t value);
NQ_EXPORT size_t NQPutInt64LE(void* buffer, int64_t value);
NQ_EXPORT size_t NQPutInt64BE(void* buffer, int64_t value);

NQ_EXPORT size_t NQPutUint64(void* buffer, uint64_t value);
NQ_EXPORT size_t NQPutUint64LE(void* buffer, uint64_t value);
NQ_EXPORT size_t NQPutUint64BE(void* buffer, uint64_t value);


typedef struct NQBufferBuilder {
  uint32_t flags;
  size_t size;
  uint8_t* data;
} NQBufferBuilder;

NQ_EXPORT void NQBufferBuilder_init(NQBufferBuilder* builder, uint8_t* data, size_t size);
NQ_EXPORT bool NQBufferBuilder_isOverflow(const NQBufferBuilder* builder);
NQ_EXPORT uint8_t* NQBufferBuilder_currentPosition(NQBufferBuilder* builder);

NQ_EXPORT size_t NQBufferBuilder_write(NQBufferBuilder* builder, const void* data, size_t size);

NQ_EXPORT size_t NQBufferBuilder_writeInt8(NQBufferBuilder* builder, int8_t value);
NQ_EXPORT size_t NQBufferBuilder_writeUint8(NQBufferBuilder* builder, uint8_t value);

NQ_EXPORT size_t NQBufferBuilder_writeInt16(NQBufferBuilder* builder, int16_t value);
NQ_EXPORT size_t NQBufferBuilder_writeInt16LE(NQBufferBuilder* builder, int16_t value);
NQ_EXPORT size_t NQBufferBuilder_writeInt16BE(NQBufferBuilder* builder, int16_t value);

NQ_EXPORT size_t NQBufferBuilder_writeUint16(NQBufferBuilder* builder, uint16_t value);
NQ_EXPORT size_t NQBufferBuilder_writeUint16LE(NQBufferBuilder* builder, uint16_t value);
NQ_EXPORT size_t NQBufferBuilder_writeUint16BE(NQBufferBuilder* builder, uint16_t value);

NQ_EXPORT size_t NQBufferBuilder_writeInt24(NQBufferBuilder* builder, int32_t value);
NQ_EXPORT size_t NQBufferBuilder_writeInt24LE(NQBufferBuilder* builder, int32_t value);
NQ_EXPORT size_t NQBufferBuilder_writeInt24BE(NQBufferBuilder* builder, int32_t value);

NQ_EXPORT size_t NQBufferBuilder_writeUint24(NQBufferBuilder* builder, uint32_t value);
NQ_EXPORT size_t NQBufferBuilder_writeUint24LE(NQBufferBuilder* builder, uint32_t value);
NQ_EXPORT size_t NQBufferBuilder_writeUint24BE(NQBufferBuilder* builder, uint32_t value);

NQ_EXPORT size_t NQBufferBuilder_writeInt32(NQBufferBuilder* builder, int32_t value);
NQ_EXPORT size_t NQBufferBuilder_writeInt32LE(NQBufferBuilder* builder, int32_t value);
NQ_EXPORT size_t NQBufferBuilder_writeInt32BE(NQBufferBuilder* builder, int32_t value);

NQ_EXPORT size_t NQBufferBuilder_writeUint32(NQBufferBuilder* builder, uint32_t value);
NQ_EXPORT size_t NQBufferBuilder_writeUint32LE(NQBufferBuilder* builder, uint32_t value);
NQ_EXPORT size_t NQBufferBuilder_writeUint32BE(NQBufferBuilder* builder, uint32_t value);

NQ_EXPORT size_t NQBufferBuilder_writeInt64(NQBufferBuilder* builder, int64_t value);
NQ_EXPORT size_t NQBufferBuilder_writeInt64LE(NQBufferBuilder* builder, int64_t value);
NQ_EXPORT size_t NQBufferBuilder_writeInt64BE(NQBufferBuilder* builder, int64_t value);

NQ_EXPORT size_t NQBufferBuilder_writeUint64(NQBufferBuilder* builder, uint64_t value);
NQ_EXPORT size_t NQBufferBuilder_writeUint64LE(NQBufferBuilder* builder, uint64_t value);
NQ_EXPORT size_t NQBufferBuilder_writeUint64BE(NQBufferBuilder* builder, uint64_t value);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_BUFFERBUILDER_H */
