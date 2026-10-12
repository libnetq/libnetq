/*
 * MIT License
 *
 * Copyright (c) 2021-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/BufferBuilder.h"

#include <libnetq/Assert.h>

size_t NQGetInt16(const void* buffer, int16_t* value)
{
  NQMemcpy(value, buffer, sizeof(*value));
  return sizeof(*value);
}

size_t NQGetInt16LE(const void* buffer, int16_t* value)
{
  NQMemcpy(value, buffer, sizeof(*value));
  *value = NQHostToLE16(*value);
  return sizeof(*value);
}

size_t NQGetInt16BE(const void* buffer, int16_t* value)
{
  NQMemcpy(value, buffer, sizeof(*value));
  *value = NQHostToBE16(*value);
  return sizeof(*value);
}

size_t NQGetUint16(const void* buffer, uint16_t* value)
{
  NQMemcpy(value, buffer, sizeof(*value));
  return sizeof(*value);
}

size_t NQGetUint16LE(const void* buffer, uint16_t* value)
{
  NQMemcpy(value, buffer, sizeof(*value));
  *value = NQHostToLE16(*value);
  return sizeof(*value);
}

size_t NQGetUint16BE(const void* buffer, uint16_t* value)
{
  NQMemcpy(value, buffer, sizeof(*value));
  *value = NQHostToBE16(*value);
  return sizeof(*value);
}

size_t NQGetInt24(const void* buffer, int32_t* value)
{
#ifdef NQ_CPU_BIG_ENDIAN
  NQMemcpy(((uint8_t*)value) + 1, buffer, 3);
#else
  NQMemcpy(value, buffer, 3);
#endif
  if (*value & 0x00800000)
    *value |= 0xFF000000;
  return 3;
}

size_t NQGetInt24LE(const void* buffer, int32_t* value)
{
  NQMemcpy(value, buffer, 3);
  *value = NQLEToHost32(*value);
  if (*value & 0x00800000)
    *value |= 0xFF000000;
  return 3;
}

size_t NQGetInt24BE(const void* buffer, int32_t* value)
{
  NQMemcpy(((uint8_t*)value) + 1, buffer, 3);
  *value = NQBEToHost32(*value);
  if (*value & 0x00800000)
    *value |= 0xFF000000;
  return 3;
}

size_t NQGetUint24(const void* buffer, uint32_t* value)
{
#ifdef NQ_CPU_BIG_ENDIAN
  NQMemcpy(((uint8_t*)value) + 1, buffer, 3);
#else
  NQMemcpy(value, buffer, 3);
#endif
  return 3;
}

size_t NQGetUint24LE(const void* buffer, uint32_t* value)
{
  NQMemcpy(value, buffer, 3);
  *value = NQLEToHost32(*value);
  return 3;
}

size_t NQGetUint24BE(const void* buffer, uint32_t* value)
{
  NQMemcpy(((uint8_t*)value) + 1, buffer, 3);
  *value = NQBEToHost32(*value);
  return 3;
}

size_t NQGetInt32(const void* buffer, int32_t* value)
{
  NQMemcpy(value, buffer, sizeof(*value));
  return sizeof(*value);
}

size_t NQGetInt32LE(const void* buffer, int32_t* value)
{
  NQMemcpy(value, buffer, sizeof(*value));
  *value = NQHostToLE32(*value);
  return sizeof(*value);
}

size_t NQGetInt32BE(const void* buffer, int32_t* value)
{
  NQMemcpy(value, buffer, sizeof(*value));
  *value = NQHostToBE32(*value);
  return sizeof(*value);
}

size_t NQGetUint32(const void* buffer, uint32_t* value)
{
  NQMemcpy(value, buffer, sizeof(*value));
  return sizeof(*value);
}

size_t NQGetUint32LE(const void* buffer, uint32_t* value)
{
  NQMemcpy(value, buffer, sizeof(*value));
  *value = NQHostToLE32(*value);
  return sizeof(*value);
}

size_t NQGetUint32BE(const void* buffer, uint32_t* value)
{
  NQMemcpy(value, buffer, sizeof(*value));
  *value = NQHostToBE32(*value);
  return sizeof(*value);
}

size_t NQGetInt64(const void* buffer, int64_t* value)
{
  NQMemcpy(value, buffer, sizeof(*value));
  return sizeof(*value);
}

size_t NQGetInt64LE(const void* buffer, int64_t* value)
{
  NQMemcpy(value, buffer, sizeof(*value));
  *value = NQHostToLE64(*value);
  return sizeof(*value);
}

size_t NQGetInt64BE(const void* buffer, int64_t* value)
{
  NQMemcpy(value, buffer, sizeof(*value));
  *value = NQHostToBE64(*value);
  return sizeof(*value);
}

size_t NQGetUint64(const void* buffer, uint64_t* value)
{
  NQMemcpy(value, buffer, sizeof(*value));
  return sizeof(*value);
}

size_t NQGetUint64LE(const void* buffer, uint64_t* value)
{
  NQMemcpy(value, buffer, sizeof(*value));
  *value = NQHostToLE64(*value);
  return sizeof(*value);
}

size_t NQGetUint64BE(const void* buffer, uint64_t* value)
{
  NQMemcpy(value, buffer, sizeof(*value));
  *value = NQHostToBE64(*value);
  return sizeof(*value);
}

size_t NQPutBytes(void* buffer, const uint8_t* data, size_t size)
{
  NQMemcpy(buffer, data, size);
  return size;
}

size_t NQPutInt8(void* buffer, int8_t value)
{
  *((int8_t*)buffer) = value;
  return sizeof(value);
}

size_t NQPutUint8(void* buffer, uint8_t value)
{
  *((uint8_t*)buffer) = value;
  return sizeof(value);
}

size_t NQPutInt16(void* buffer, int16_t value)
{
  NQMemcpy(buffer, &value, sizeof(value));
  return sizeof(value);
}

size_t NQPutInt16LE(void* buffer, int16_t value)
{
  value = NQHostToLE16(value);
  NQMemcpy(buffer, &value, sizeof(value));
  return sizeof(value);
}

size_t NQPutInt16BE(void* buffer, int16_t value)
{
  value = NQHostToBE16(value);
  NQMemcpy(buffer, &value, sizeof(value));
  return sizeof(value);
}

size_t NQPutUint16(void* buffer, uint16_t value)
{
  NQMemcpy(buffer, &value, sizeof(value));
  return sizeof(value);
}

size_t NQPutUint16LE(void* buffer, uint16_t value)
{
  value = NQHostToLE16(value);
  NQMemcpy(buffer, &value, sizeof(value));
  return sizeof(value);
}

size_t NQPutUint16BE(void* buffer, uint16_t value)
{
  value = NQHostToBE16(value);
  NQMemcpy(buffer, &value, sizeof(value));
  return sizeof(value);
}

size_t NQPutInt24(void* buffer, int32_t value)
{
#ifdef NQ_CPU_BIG_ENDIAN
  NQMemcpy(buffer, ((uint8_t*)&value) + 1, 3);
#else
  NQMemcpy(buffer, &value, 3);
#endif
  return 3;
}

size_t NQPutInt24LE(void* buffer, int32_t value)
{
  value = NQHostToLE32(value);
  NQMemcpy(buffer, &value, 3);
  return 3;
}

size_t NQPutInt24BE(void* buffer, int32_t value)
{
  value = NQHostToBE32(value);
  NQMemcpy(buffer, ((uint8_t*)&value) + 1, 3);
  return 3;
}

size_t NQPutUint24(void* buffer, uint32_t value)
{
#ifdef NQ_CPU_BIG_ENDIAN
  NQMemcpy(buffer, ((uint8_t*)&value) + 1, 3);
#else
  NQMemcpy(buffer, &value, 3);
#endif
  return 3;
}

size_t NQPutUint24LE(void* buffer, uint32_t value)
{
  value = NQHostToLE32(value);
  NQMemcpy(buffer, &value, 3);
  return 3;
}

size_t NQPutUint24BE(void* buffer, uint32_t value)
{
  value = NQHostToBE32(value);
  NQMemcpy(buffer, ((uint8_t*)&value) + 1, 3);
  return 3;
}

size_t NQPutInt32(void* buffer, int32_t value)
{
  NQMemcpy(buffer, &value, sizeof(value));
  return sizeof(value);
}

size_t NQPutInt32LE(void* buffer, int32_t value)
{
  value = NQHostToLE32(value);
  NQMemcpy(buffer, &value, sizeof(value));
  return sizeof(value);
}

size_t NQPutInt32BE(void* buffer, int32_t value)
{
  value = NQHostToBE32(value);
  NQMemcpy(buffer, &value, sizeof(value));
  return sizeof(value);
}

size_t NQPutUint32(void* buffer, uint32_t value)
{
  NQMemcpy(buffer, &value, sizeof(value));
  return sizeof(value);
}

size_t NQPutUint32LE(void* buffer, uint32_t value)
{
  value = NQHostToLE32(value);
  NQMemcpy(buffer, &value, sizeof(value));
  return sizeof(value);
}

size_t NQPutUint32BE(void* buffer, uint32_t value)
{
  value = NQHostToBE32(value);
  NQMemcpy(buffer, &value, sizeof(value));
  return sizeof(value);
}

size_t NQPutInt64(void* buffer, int64_t value)
{
  NQMemcpy(buffer, &value, sizeof(value));
  return sizeof(value);
}

size_t NQPutInt64LE(void* buffer, int64_t value)
{
  value = NQHostToLE64(value);
  NQMemcpy(buffer, &value, sizeof(value));
  return sizeof(value);
}

size_t NQPutInt64BE(void* buffer, int64_t value)
{
  value = NQHostToBE64(value);
  NQMemcpy(buffer, &value, sizeof(value));
  return sizeof(value);
}

size_t NQPutUint64(void* buffer, uint64_t value)
{
  NQMemcpy(buffer, &value, sizeof(value));
  return sizeof(value);
}

size_t NQPutUint64LE(void* buffer, uint64_t value)
{
  value = NQHostToLE64(value);
  NQMemcpy(buffer, &value, sizeof(value));
  return sizeof(value);
}

size_t NQPutUint64BE(void* buffer, uint64_t value)
{
  value = NQHostToBE64(value);
  NQMemcpy(buffer, &value, sizeof(value));
  return sizeof(value);
}

#define BUFFER_OVERFLOW (1 << 0)

void NQBufferBuilder_init(NQBufferBuilder* builder, uint8_t* data, size_t size)
{
  builder->flags = 0;
  builder->data = data;
  builder->size = size;
}

bool NQBufferBuilder_isOverflow(const NQBufferBuilder* builder)
{
  return (builder->flags & BUFFER_OVERFLOW) ? true : false;
}

uint8_t* NQBufferBuilder_currentPosition(NQBufferBuilder* builder)
{
  return builder->data;
}

size_t NQBufferBuilder_write(NQBufferBuilder* builder, const void* data, size_t size)
{
  size_t n;

  if (size <= builder->size)
    n = size;
  else {
    n = builder->size;
    builder->flags |= BUFFER_OVERFLOW;
    if (n == 0)
      return size;
  }

  NQMemcpy(builder->data, data, n);
  builder->data += n;
  builder->size -= n;

  return size;
}

size_t NQBufferBuilder_writeInt8(NQBufferBuilder* builder, int8_t value)
{
  return NQBufferBuilder_write(builder, &value, sizeof(value));
}

size_t NQBufferBuilder_writeUint8(NQBufferBuilder* builder, uint8_t value)
{
  return NQBufferBuilder_write(builder, &value, sizeof(value));
}

size_t NQBufferBuilder_writeInt16(NQBufferBuilder* builder, int16_t value)
{
  return NQBufferBuilder_write(builder, &value, sizeof(value));
}

size_t NQBufferBuilder_writeInt16LE(NQBufferBuilder* builder, int16_t value)
{
  value = NQHostToLE16(value);
  return NQBufferBuilder_write(builder, &value, sizeof(value));
}

size_t NQBufferBuilder_writeInt16BE(NQBufferBuilder* builder, int16_t value)
{
  value = NQHostToBE16(value);
  return NQBufferBuilder_write(builder, &value, sizeof(value));
}

size_t NQBufferBuilder_writeUint16(NQBufferBuilder* builder, uint16_t value)
{
  return NQBufferBuilder_write(builder, &value, sizeof(value));
}

size_t NQBufferBuilder_writeUint16LE(NQBufferBuilder* builder, uint16_t value)
{
  value = NQHostToLE16(value);
  return NQBufferBuilder_write(builder, &value, sizeof(value));
}

size_t NQBufferBuilder_writeUint16BE(NQBufferBuilder* builder, uint16_t value)
{
  value = NQHostToBE16(value);
  return NQBufferBuilder_write(builder, &value, sizeof(value));
}

size_t NQBufferBuilder_writeInt24(NQBufferBuilder* builder, int32_t value)
{
#ifdef NQ_CPU_BIG_ENDIAN
  return NQBufferBuilder_write(builder, ((uint8_t*)&value) + 1, 3);
#else
  return NQBufferBuilder_write(builder, &value, 3);
#endif
}

size_t NQBufferBuilder_writeInt24LE(NQBufferBuilder* builder, int32_t value)
{
  value = NQHostToLE32(value);
  return NQBufferBuilder_write(builder, &value, 3);
}

size_t NQBufferBuilder_writeInt24BE(NQBufferBuilder* builder, int32_t value)
{
  value = NQHostToBE32(value);
  return NQBufferBuilder_write(builder, ((uint8_t*)&value) + 1, 3);
}

size_t NQBufferBuilder_writeUint24(NQBufferBuilder* builder, uint32_t value)
{
#ifdef NQ_CPU_BIG_ENDIAN
  return NQBufferBuilder_write(builder, ((uint8_t*)&value) + 1, 3);
#else
  return NQBufferBuilder_write(builder, &value, 3);
#endif
}

size_t NQBufferBuilder_writeUint24LE(NQBufferBuilder* builder, uint32_t value)
{
  value = NQHostToLE32(value);
  return NQBufferBuilder_write(builder, &value, 3);
}

size_t NQBufferBuilder_writeUint24BE(NQBufferBuilder* builder, uint32_t value)
{
  value = NQHostToBE32(value);
  return NQBufferBuilder_write(builder, ((uint8_t*)&value) + 1, 3);
}

size_t NQBufferBuilder_writeInt32(NQBufferBuilder* builder, int32_t value)
{
  return NQBufferBuilder_write(builder, &value, sizeof(value));
}

size_t NQBufferBuilder_writeInt32LE(NQBufferBuilder* builder, int32_t value)
{
  value = NQHostToLE32(value);
  return NQBufferBuilder_write(builder, &value, sizeof(value));
}

size_t NQBufferBuilder_writeInt32BE(NQBufferBuilder* builder, int32_t value)
{
  value = NQHostToBE32(value);
  return NQBufferBuilder_write(builder, &value, sizeof(value));
}

size_t NQBufferBuilder_writeUint32(NQBufferBuilder* builder, uint32_t value)
{
  return NQBufferBuilder_write(builder, &value, sizeof(value));
}

size_t NQBufferBuilder_writeUint32LE(NQBufferBuilder* builder, uint32_t value)
{
  value = NQHostToLE32(value);
  return NQBufferBuilder_write(builder, &value, sizeof(value));
}

size_t NQBufferBuilder_writeUint32BE(NQBufferBuilder* builder, uint32_t value)
{
  value = NQHostToBE32(value);
  return NQBufferBuilder_write(builder, &value, sizeof(value));
}

size_t NQBufferBuilder_writeInt64(NQBufferBuilder* builder, int64_t value)
{
  return NQBufferBuilder_write(builder, &value, sizeof(value));
}

size_t NQBufferBuilder_writeInt64LE(NQBufferBuilder* builder, int64_t value)
{
  value = NQHostToLE64(value);
  return NQBufferBuilder_write(builder, &value, sizeof(value));
}

size_t NQBufferBuilder_writeInt64BE(NQBufferBuilder* builder, int64_t value)
{
  value = NQHostToBE64(value);
  return NQBufferBuilder_write(builder, &value, sizeof(value));
}

size_t NQBufferBuilder_writeUint64(NQBufferBuilder* builder, uint64_t value)
{
  return NQBufferBuilder_write(builder, &value, sizeof(value));
}

size_t NQBufferBuilder_writeUint64LE(NQBufferBuilder* builder, uint64_t value)
{
  value = NQHostToLE64(value);
  return NQBufferBuilder_write(builder, &value, sizeof(value));
}

size_t NQBufferBuilder_writeUint64BE(NQBufferBuilder* builder, uint64_t value)
{
  value = NQHostToBE64(value);
  return NQBufferBuilder_write(builder, &value, sizeof(value));
}
