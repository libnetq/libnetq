/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_CLASS_NAME "NQLogCircular"
#define NQ_LOG_TAG NQ_CLASS_NAME

#include "config.h"
#include "libnetq/LogCircular.h"

#include <libnetq/Malloc.h>
#include <libnetq/Limits.h>
#include <libnetq/MinMax.h>
#include <libnetq/string/Sprintf.h>
#include <libnetq/string/String.h>
#include <libnetq/Time.h>
#include <libnetq/Assert.h>

// TODO: Add time and level

#define NQ_LOGC_IS_FULL (1 << 0)

typedef struct NQLogCircularItem {
  uint16_t length;
  char characters[1];
} NQLogCircularItem;

struct NQLogCircular {
  NQLogCircularItem** items;
  uint32_t flags;
  uint16_t capacity;
  uint16_t position;
};

static size_t NQLogCircular_getIndex(const NQLogCircular* log, size_t index)
{
  NQ_ASSERT(index < NQLogCircular_count(log));

  if (log->flags & NQ_LOGC_IS_FULL)
    index += log->position;
  index %= log->capacity;

  return index;
}

static const struct NQLogCircularItem* NQLogCircular_at(const NQLogCircular* log, size_t index)
{
  index = NQLogCircular_getIndex(log, index);
  return log->items[index];
}

#define LOG_PREFIX_SIZE 15

static char* NQLogCircular_add_prefix(char* buffer, NQLogLevel level)
{
  // "13:02:50.071 I *"
  size_t n;

  n = NQTimeMsFormat(NQGetTimeMs(), NQ_DT_RFC3339_TIMEMS, buffer, LOG_PREFIX_SIZE);
  buffer[n++] = ' ';
  buffer[n++] = NQLogLevelToChar(level);
  buffer[n++] = ' ';

  NQ_ASSERT(n == LOG_PREFIX_SIZE);
  return buffer + n;
}

NQLogCircular* NQLogCircular_create(size_t capacity)
{
  NQLogCircular* log;
  if (NQ_UINT16_MAX < capacity)
    return NULL;

  log = (NQLogCircular*)NQZeroMalloc(sizeof(NQLogCircular));
  if (log == NULL)
    return NULL;

  log->items = (NQLogCircularItem**)NQZeroMalloc(sizeof(NQLogCircularItem**) * capacity);
  if (log->items == NULL) {
    NQFree((void*)log);
    return NULL;
  }
  log->capacity = (uint16_t)capacity;
  
  return log;
}

void NQLogCircular_destroy(NQLogCircular* log)
{
  NQ_ASSERT(log != NULL);

  size_t index;
  struct NQLogCircularItem* iter;

  for (index = 0; index < log->capacity; index++) {
    iter = log->items[index];
    if (iter != NULL)
      NQFree((void*)iter);
  }

  NQFree((void*)log->items);
  NQFree((void*)log);
}

size_t NQLogCircular_print(NQLogCircular* log, NQLogLevel level, const char* format, ...)
{
  size_t result;
  va_list args;
  va_start(args, format);
  result = NQLogCircular_vprint(log, level, format, args);
  va_end(args);
  return result;
}

size_t NQLogCircular_vprint(NQLogCircular* log, NQLogLevel level, const char* format, va_list args)
{
  NQ_ASSERT(log && format);

  int length;
  struct NQLogCircularItem* item;
  char* characters;
  size_t size;

#ifdef NQ_OS_WINDOWS
  length = vsnprintf(NULL, 0, format, args);
#else
  va_list argsCopy;
  va_copy(argsCopy, args);
  length = vsnprintf(NULL, 0, format, argsCopy);
#endif

  size = length + LOG_PREFIX_SIZE;
  size = NQGetClamp(size, NQ_UINT16_MIN, NQ_UINT16_MAX);
  item = (struct NQLogCircularItem*)NQMalloc(sizeof(struct NQLogCircularItem*) + size);
  if (item == NULL)
    return 0;

  characters = NQLogCircular_add_prefix(item->characters, level);
  length = vsnprintf(characters, (size_t)length + 1, format, args);
  item->length = (uint16_t)(LOG_PREFIX_SIZE + length);

  NQ_ASSERT(item->characters[size] == '\0');
  if (log->flags & NQ_LOGC_IS_FULL)
    NQFree((void*)log->items[log->position]);
  log->items[log->position++] = item;

  if (log->position >= log->capacity) {
    log->position = 0;
    log->flags |= NQ_LOGC_IS_FULL;
  }

  return (size_t)item->length;
}

size_t NQLogCircular_count(const NQLogCircular* log)
{
  NQ_ASSERT(log != NULL);
  if (log->flags & NQ_LOGC_IS_FULL)
    return log->capacity;

  return log->position;
}

bool NQLogCircular_isFull(const NQLogCircular* log)
{
  NQ_ASSERT(log != NULL);
  return log->flags & NQ_LOGC_IS_FULL;
}

const char* NQLogCircular_charactersAt(const NQLogCircular* log, size_t index)
{
  return NQLogCircular_at(log, index)->characters;
}

size_t NQLogCircular_lengthAt(const NQLogCircular* log, size_t index)
{
  return NQLogCircular_at(log, index)->length;
}
