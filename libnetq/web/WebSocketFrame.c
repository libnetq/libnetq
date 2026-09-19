/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/web/WebSocketFrame.h"

#include <libnetq/String.h>
#include <libnetq/BufferBuilder.h>
#include <libnetq/Malloc.h>
#include <libnetq/Limits.h>
#include <libnetq/MinMax.h>
#include <libnetq/string/Sprintf.h>
#include <libnetq/Assert.h>
#include <libnetq/web/WebSocketTypes.h>

#define WS_FIN_MASK (0x80)
#define WS_RSV_MASK (0x70)
#define WS_RSV_SHIFT (4)
#define WS_OPCODE_MASK (0x0f)
#define WS_MASK_MASK (0x80)
#define WS_LEN1_MASK (0x7f)
#define WS_LEN16_MARKER 126
#define WS_LEN64_MARKER 127

#define NQ_WEBSOCKET_BUFFER_MAX (NQ_UINT64_MAX - NQ_WEBSOCKET_HEADER_MAX)
#define NQ_WEBSOCKET_LENGTH1_MAX 125

int NQWebSocketHeaderParse(const uint8_t* data, size_t size, NQWebSocketHeader* result)
{
  if (size < 2) {
    return 0;
  }

  const uint8_t* ptr = data;
  uint8_t flags1 = *ptr++;
  size--;

  uint8_t flags2 = *ptr++;
  size--;

  bool fin = (flags1 & WS_FIN_MASK) ? true : false;
  uint8_t rsv = (flags1 & WS_RSV_MASK) >> WS_RSV_SHIFT;

  uint8_t opcode = flags1 & WS_OPCODE_MASK;
  uint64_t payloadSize = flags2 & WS_LEN1_MASK;
  bool masked = (flags2 & WS_MASK_MASK) ? true : false;

  if (payloadSize == WS_LEN16_MARKER) {
    if (size < 2) {
      return 0;
    }

    uint16_t len16;
    NQGetUint16BE(ptr, &len16);
    ptr += sizeof(len16);
    size -= sizeof(len16);

    payloadSize = len16;
  }
  else if (payloadSize == 127) {
    if (size < 8) {
      return 0;
    }

    NQGetUint64BE(ptr, &payloadSize);
    ptr += sizeof(payloadSize);
    size -= sizeof(payloadSize);
  }

  uint8_t mkey[4];
  if (masked) {
    if (size < sizeof(mkey)) {
      return 0;
    }
    (void)memcpy(mkey, ptr, sizeof(mkey));
    ptr += sizeof(mkey);
    size -= sizeof(mkey);
  }
  else {
    mkey[0] = 0;
    mkey[1] = 0;
    mkey[2] = 0;
    mkey[3] = 0;
  }

  if (result) {
    result->fin = fin;
    result->rsv = rsv;
    result->hasMask = masked;
    (void)memcpy(result->mask, mkey, sizeof(mkey));
    result->opcode = opcode;
    result->payloadSize = payloadSize;
  }

  return (int)(ptr - data);
}

bool NQWebSocketFrameParse(uint8_t* data, size_t size, NQWebSocketFrame* frame)
{
  NQWebSocketHeader header;
  int headerSize = NQWebSocketHeaderParse(data, size, &header);
  if (headerSize <= 0)
    return false;

  if (header.rsv) {
    return false;
  }

  switch (header.opcode) {
  case kNQWebSocketOpcodeText:
  case kNQWebSocketOpcodeBinary:
    break;

  case kNQWebSocketOpcodeConnectionClose:
    if (header.payloadSize != 2)
      return false;
    break;

  default:
    return false;
  }

  size_t payloadSize = (size_t)header.payloadSize;
  if (header.payloadSize != payloadSize)
    return false;

  if ((headerSize + header.payloadSize) != size) {
    return false;
  }

  uint8_t* payload = data + headerSize;
  if (header.hasMask) {
    NQWebSocketFramePayloadUnmask(header.mask, payload, header.payloadSize);
  }

  frame->fin = header.fin;
  frame->hasMask = header.hasMask;
  (void)memcpy(&frame->mask, header.mask, sizeof(header.mask));
  frame->opcode = header.opcode;
  frame->payload = payload;
  frame->payloadSize = header.payloadSize;

  return true;
}

#define toHeaderPtr(payloadPtr) ((payloadPtr) - NQ_WEBSOCKET_HEADER_MAX)
#define toPayloadPtr(headerPtr) ((headerPtr) + NQ_WEBSOCKET_HEADER_MAX)

void NQWebSocketBuffer_init(NQWebSocketBuffer* thiz)
{
  thiz->payloadPtr = NULL;
  thiz->payloadLen = 0;
  thiz->payloadCapacity = 0;
  thiz->headerSize = 0;
}

void NQWebSocketBuffer_finalize(NQWebSocketBuffer* thiz)
{
  if (thiz->payloadPtr) {
    NQFree(toHeaderPtr(thiz->payloadPtr));
  }
}

bool NQWebSocketBuffer_reserve(NQWebSocketBuffer* thiz, size_t size)
{
  if (thiz->payloadCapacity >= size)
    return true;

  size_t allocSize = NQ_WEBSOCKET_HEADER_MAX + size;
  if (allocSize < size)
    return false;

  uint8_t* headerPtr = NQMalloc(allocSize);
  if (headerPtr == NULL)
    return false;

  uint8_t* newPayloadPtr = toPayloadPtr(headerPtr);
  if (thiz->payloadLen != 0) {
    NQ_ASSERT(thiz->payloadPtr != NULL);
    NQ_ASSERT(thiz->payloadCapacity >= thiz->payloadLen);
    memcpy(newPayloadPtr, thiz->payloadPtr, thiz->payloadLen);
    NQFree(toHeaderPtr(thiz->payloadPtr));
  }

  thiz->payloadPtr = newPayloadPtr;
  thiz->payloadCapacity = size;
  return true;
}

int NQWebSocketBuffer_printf(NQWebSocketBuffer* thiz, const char* format, ...)
{
  va_list list;
  va_start(list, format);
  int ret = NQWebSocketBuffer_vprintf(thiz, format, list);
  va_end(list);
  return ret;
}

static bool payloadCapacityResize(NQWebSocketBuffer* thiz, uint64_t newPayloadCapacity)
{
  NQ_ASSERT(newPayloadCapacity > thiz->payloadCapacity);
  NQ_ASSERT(newPayloadCapacity <= NQ_WEBSOCKET_BUFFER_MAX);

  if ((newPayloadCapacity * 2) > newPayloadCapacity)
    newPayloadCapacity *= 2;

  uint8_t* newHeaderPtr = (uint8_t*)NQMalloc(NQ_WEBSOCKET_HEADER_MAX + newPayloadCapacity);
  if (newHeaderPtr == NULL)
    return false;

  uint8_t* newPayloadPtr = toPayloadPtr(newHeaderPtr);
  if (thiz->payloadLen)
    memcpy(newPayloadPtr, thiz->payloadPtr, thiz->payloadLen);
  if (thiz->payloadPtr)
    NQFree(toHeaderPtr(thiz->payloadPtr));

  thiz->payloadCapacity = newPayloadCapacity;
  thiz->payloadPtr = newPayloadPtr;

  return true;
}

int NQWebSocketBuffer_vprintf(NQWebSocketBuffer* thiz, const char* format, va_list list)
{
  va_list listCopy;

  va_copy(listCopy, list);
  int n = vsnprintf((char*)thiz->payloadPtr + thiz->payloadLen, thiz->payloadCapacity - thiz->payloadLen, format, listCopy);
  va_end(listCopy);

  if (n <= 0)
    return n;

  uint64_t newPayloadLen = thiz->payloadLen + n;
  // For newPayloadLen == payloadCapacity need +1 byte for NIL
  if (newPayloadLen < thiz->payloadCapacity) {
    thiz->payloadLen = newPayloadLen;
    return n;
  }

  if (!payloadCapacityResize(thiz, newPayloadLen + 1))
    return -1;

  int ret = vsnprintf((char*)thiz->payloadPtr + thiz->payloadLen, thiz->payloadCapacity - thiz->payloadLen, format, list);
  NQ_ASSERT(ret == n);
  if (ret <= 0)
    return ret;

  thiz->payloadLen += ret;
  NQ_ASSERT(thiz->payloadLen == newPayloadLen);
  return ret;
}

int NQWebSocketBuffer_write(NQWebSocketBuffer* thiz, const void* data, size_t length)
{
  length = NQGetMin(NQ_INT32_MAX, length);

  uint64_t newPayloadLen = thiz->payloadLen + (int)length;
  if (newPayloadLen < thiz->payloadLen) {
    if (thiz->payloadLen >= NQ_WEBSOCKET_BUFFER_MAX)
      return -1;
    newPayloadLen = NQ_WEBSOCKET_BUFFER_MAX;
    length = NQ_WEBSOCKET_BUFFER_MAX - thiz->payloadLen;
  }
  if (newPayloadLen > thiz->payloadCapacity) {
    if (!payloadCapacityResize(thiz, newPayloadLen))
      return -1;
  }

  memcpy(thiz->payloadPtr + thiz->payloadLen, data, length);
  thiz->payloadLen = newPayloadLen;
  return (int)length;
}

void NQWebSocketBuffer_complete(NQWebSocketBuffer* thiz, uint8_t opcode, bool fin, const uint8_t* mask)
{
  uint8_t flag1;
  uint8_t flag2;
  uint16_t len2;
  uint64_t len8;

  if (thiz->payloadPtr == NULL)
    return;

  flag1 = opcode;
  if (fin)
    flag1 |= WS_FIN_MASK;

  thiz->headerSize = sizeof(flag1) + sizeof(flag2);

  if (thiz->payloadLen <= NQ_WEBSOCKET_LENGTH1_MAX) {
    flag2 = (WS_LEN1_MASK & thiz->payloadLen);
    len2 = len8 = 0;
  }
  else if (thiz->payloadLen <= NQ_UINT16_MAX) {
    flag2 = (WS_LEN1_MASK & WS_LEN16_MARKER);
    len2 = (uint16_t)thiz->payloadLen;
    len8 = 0;
    thiz->headerSize += sizeof(len2);
  }
  else {
    flag2 = (WS_LEN1_MASK & WS_LEN64_MARKER);
    len2 = 0;
    len8 = (uint64_t)thiz->payloadLen;
    thiz->headerSize += sizeof(len8);
  }

  if (mask) {
    thiz->headerSize += 4;
    flag2 |= WS_MASK_MASK;
  }

  uint8_t* headerPtr = thiz->payloadPtr - thiz->headerSize;
  *headerPtr++ = flag1;
  *headerPtr++ = flag2;

  if (len2 != 0)
    headerPtr += NQPutUint16BE(headerPtr, len2);
  else if (len8 != 0)
    headerPtr += NQPutUint64BE(headerPtr, len8);

  if (mask) {
    (void)memcpy(headerPtr, mask, 4);
    NQWebSocketFramePayloadUnmask(mask, thiz->payloadPtr, thiz->payloadLen);
    NQ_ASSERT(headerPtr + 4 == thiz->payloadPtr);
  }
}

void NQWebSocketBuffer_reset(NQWebSocketBuffer* thiz)
{
  thiz->headerSize = 0;
  thiz->payloadLen = 0;
}
