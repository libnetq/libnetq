/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_WEB_WEBSOCKETFRAME_H
#define _LIBNETQ_WEB_WEBSOCKETFRAME_H

#include <libnetq/Basic.h>
#include <libnetq/VA.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  kNQWebSocketOpcodeContinuation = 0,
  kNQWebSocketOpcodeText = 1,
  kNQWebSocketOpcodeBinary = 2,
  kNQWebSocketOpcodeConnectionClose = 8,
  kNQWebSocketOpcodePing = 9,
  kNQWebSocketOpcodePong = 10,
};

typedef struct NQWebSocketHeader NQWebSocketHeader;
struct NQWebSocketHeader {
  uint64_t payloadSize;
  uint8_t mask[4];
  uint8_t rsv;
  uint8_t opcode;
  bool fin;
  bool hasMask;
};

NQ_EXPORT int NQWebSocketHeaderParse(const uint8_t* data, size_t size, NQWebSocketHeader* result);

typedef struct NQWebSocketFrame NQWebSocketFrame;
struct NQWebSocketFrame {
  uint8_t opcode;
  bool fin;
  bool hasMask;
  uint32_t mask;
  size_t payloadSize;
  uint8_t* payload;
};

NQ_EXPORT bool NQWebSocketFrameParse(uint8_t* data, size_t size, NQWebSocketFrame* frame);

typedef struct NQWebSocketUnmaskPayload NQWebSocketUnmaskPayload;
struct NQWebSocketUnmaskPayload {
  uint8_t mask[4];
  uint8_t position;
};

static inline void NQWebSocketUnmaskPayload_init(NQWebSocketUnmaskPayload* thiz, const uint8_t mask[4])
{
  thiz->mask[0] = mask[0];
  thiz->mask[1] = mask[1];
  thiz->mask[2] = mask[2];
  thiz->mask[3] = mask[3];
  thiz->position = 0;
}

static inline void NQWebSocketUnmaskPayload_unmask(NQWebSocketUnmaskPayload* thiz, uint8_t* data, size_t size)
{
  for (size_t i = 0; i < size; i++)
    data[i] ^= thiz->mask[(thiz->position++) & 3];
}

static inline void NQWebSocketFramePayloadUnmask(const uint8_t mask[4], uint8_t* data, size_t size)
{
  NQWebSocketUnmaskPayload unmaskPayload;
  NQWebSocketUnmaskPayload_init(&unmaskPayload, mask);
  NQWebSocketUnmaskPayload_unmask(&unmaskPayload, data, size);
}

typedef struct NQWebSocketBuffer NQWebSocketBuffer;
struct NQWebSocketBuffer {
  uint8_t* payloadPtr;
  uint64_t payloadLen;
  uint64_t payloadCapacity;
  uint8_t headerSize;
};

NQ_EXPORT void NQWebSocketBuffer_init(NQWebSocketBuffer*);
NQ_EXPORT void NQWebSocketBuffer_finalize(NQWebSocketBuffer*);
NQ_EXPORT bool NQWebSocketBuffer_reserve(NQWebSocketBuffer*, size_t size);
NQ_EXPORT int NQWebSocketBuffer_printf(NQWebSocketBuffer*, const char* format, ...) NQ_ATTRIBUTE_PRINTF(2, 3);
NQ_EXPORT int NQWebSocketBuffer_vprintf(NQWebSocketBuffer*, const char* format, va_list);
NQ_EXPORT int NQWebSocketBuffer_write(NQWebSocketBuffer*, const void* data, size_t size);
NQ_EXPORT void NQWebSocketBuffer_complete(NQWebSocketBuffer*, uint8_t opcode, bool fin, const uint8_t* mask);
NQ_EXPORT void NQWebSocketBuffer_reset(NQWebSocketBuffer*);

static inline const uint8_t* NQWebSocketBuffer_data(const NQWebSocketBuffer* thiz)
{
  if (thiz->headerSize == 0)
    return NULL;
  return thiz->payloadPtr - thiz->headerSize;
}

static inline size_t NQWebSocketBuffer_size(const NQWebSocketBuffer* thiz)
{
  if (thiz->headerSize == 0)
    return 0;
  return thiz->payloadLen + thiz->headerSize;
}

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_WEB_WEBSOCKETFRAME_H */
