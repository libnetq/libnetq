/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_CLASS_NAME "NQHexViewer"
#define NQ_LOG_TAG NQ_CLASS_NAME

#include "config.h"
#include "libnetq/HexViewer.h"

#include <libnetq/string/StringUtil.h>
#include <libnetq/string/Sprintf.h>
#include <libnetq/Malloc.h>
#include <libnetq/Assert.h>

#define NQ_HEXVIEWER_HAS_OFFSET (1 << 1)

struct NQHexViewer {
  int flags;
  uint32_t position;
  uint32_t size;
  size_t offset;
  char buffer[16000];
};

// Offset       0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F  ASCII
// 0x00000000: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................\n

NQ_ALLOW_UNUSED
static size_t sprintLine(char* start, char** end, uint32_t offset, const void* data, size_t size)
{
  size_t i;
  char text[16];
  size_t result = 0;
  start += sprintf(start, " 0x%08X: ", offset);
  for (i = 0; i < 16; i++) {
    uint8_t b;
    if (i < size) {
      result++;
      b = ((const uint8_t*)data)[i];
      start += sprintf(start, "%02X ", b);
    }
    else {
      b = ' ';
      start += sprintf(start, "   ");
    }

    if (b < ' ' || '~' < b)
      b = '.';
    text[i] = (char)b;

    if (i == 7)
      start += sprintf(start, " ");
  }
  start += sprintf(start, " %*.s\n", (int)sizeof(text), text);

  *end = start;
  return result;
}

static void NQHexViewer_init(NQHexViewer* viewer, int mode)
{
  viewer->flags = mode & NQ_HEXVIEWER_LOWERCASTE;
  viewer->position = 0;
  viewer->offset = 0;
  viewer->size = 0;
}

NQHexViewer* NQHexViewer_create(int mode)
{
  NQHexViewer* viewer = (NQHexViewer*)NQMalloc(sizeof(NQHexViewer));
  if (viewer == NULL)
    return NULL;

  NQHexViewer_init(viewer, mode);
  return viewer;
}

void NQHexViewer_destroy(NQHexViewer* viewer)
{
  NQFree((void*)viewer);
}

void NQHexViewer_reset(NQHexViewer* viewer)
{
  NQHexViewer_init(viewer, viewer->flags);
}

void NQHexViewer_setOffset(NQHexViewer* viewer, size_t offset)
{
  viewer->offset = offset;
  viewer->flags |= NQ_HEXVIEWER_HAS_OFFSET;
}

void NQHexViewer_addData(NQHexViewer* viewer, const void* data, size_t size)
{
  if (!size)
    return;

  size_t i;

  for (i = 0; i < size; i++) {
    char* p = viewer->buffer + viewer->size;
    if ((viewer->flags & NQ_HEXVIEWER_HAS_OFFSET) && (viewer->position & 0x0F) == 0)
      p += sprintf(p, "%08llX: ", (unsigned long long)(viewer->offset + viewer->position));

    p += sprintf(p, "%02X", ((const uint8_t*)data)[i]);

    if ((viewer->position & 0x0F) == 0x0F)
      p += sprintf(p, "\n");
    else if ((viewer->position & 0x07) == 0x07)
      p += sprintf(p, "  ");
    else
      p += sprintf(p, " ");

    viewer->size = (uint32_t)(p - viewer->buffer);
    NQ_ASSERT(viewer->size < sizeof(viewer->buffer));
    viewer->position++;
  }

  if ((viewer->size + 1) < sizeof(viewer->buffer))
    viewer->buffer[viewer->size] = '\n';

  if ((viewer->size + 2) < sizeof(viewer->buffer))
    viewer->buffer[viewer->size + 1] = '\0';
}

const char* NQHexViewer_characters(const NQHexViewer* viewer)
{
  return viewer->buffer;
}

size_t NQHexViewer_length(const NQHexViewer* viewer)
{
  return viewer->size;
}
