/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/net/ChunkBuffer.h"

#include <libnetq/String.h>
#include <libnetq/Malloc.h>

void NQChunkBuffer_init(NQChunkBuffer* thiz)
{
  thiz->firstChunk = NULL;
  thiz->lastChunk = NULL;
  thiz->totalSize = 0;
}

void NQChunkBuffer_finalize(NQChunkBuffer* thiz)
{
  struct NQChunkBufferEntry* iter = thiz->firstChunk;
  while (iter != NULL) {
    struct NQChunkBufferEntry* temp = iter;
    iter = iter->next;
    NQFree(temp);
  }
}

bool NQChunkBuffer_append(NQChunkBuffer* thiz, const void* data, size_t size)
{
  struct NQChunkBufferEntry* chunk;

  if (size == 0) {
    return true;
  }

  if (thiz->lastChunk == NULL) {
    chunk = (struct NQChunkBufferEntry*)NQMalloc(sizeof(*chunk));
    if (chunk == NULL) {
      return false;
    }
    chunk->next = NULL;
    chunk->size = 0;
    thiz->firstChunk = chunk;
    thiz->lastChunk = chunk;
  }

  for (;;) {
    chunk = thiz->lastChunk;
    uint16_t remainingSize = sizeof(chunk->data) - chunk->size;
    if (remainingSize != 0) {
      if (size < remainingSize) {
        (void)NQMemcpy(chunk->data + chunk->size, data, size);
        chunk->size += size;
        break;
      }
      (void)NQMemcpy(chunk->data + chunk->size, data, remainingSize);
      chunk->size += remainingSize;
      data = (const char*)data + remainingSize;
      size -= remainingSize;
      if (size == 0) {
        break;
      }
    }
    chunk = (struct NQChunkBufferEntry*)NQMalloc(sizeof(*chunk));
    if (chunk == NULL) {
      return false;
    }
    chunk->next = thiz->lastChunk;
    chunk->size = 0;
    thiz->lastChunk = chunk;
  }

  thiz->totalSize += size;
  return true;
}
