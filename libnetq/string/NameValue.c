/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_CLASS_NAME "NQNameValue"
#define NQ_LOG_TAG NQ_CLASS_NAME

#include "config.h"
#include "libnetq/string/NameValue.h"

#include <libnetq/Malloc.h>
#include <libnetq/string/String.h>
#include <libnetq/Log.h>

void NQNameValue_finalize(NQNameValue* thiz)
{
  while (!NQListHead_isEmpty(&thiz->list)) {
    NQNameValueEntry* entry = NQ_CONTAINER_OF(thiz->list.prev, NQNameValueEntry, list);
    NQListHead_remove(&entry->list);
    NQFree(entry);
  }
}

bool NQNameValue_add(NQNameValue* thiz, const char* name, const char* value)
{
  size_t nlenz = NQStrlen(name) + 1;
  size_t vlenz = NQStrlen(value) + 1;

  NQNameValueEntry* entry = NQMalloc(sizeof(*entry) + nlenz + vlenz - 1);
  if (entry == NULL)
    return false;

  NQListHead_init(&entry->list);
  entry->name = (char*)thiz + sizeof(*entry) + nlenz;
  NQMemcpy(entry->name, name, nlenz);
  NQMemcpy(entry->value, value, vlenz);

  NQListHead_addBack(&thiz->list, &entry->list);

  return true;
}

NQNameValueEntry* NQNameValue_first(NQNameValue* thiz)
{
  if (NQListHead_isEmpty(&thiz->list))
    return NULL;
  return NQ_CONTAINER_OF(thiz->list.next, struct NQNameValueEntry, list);
}

NQNameValueEntry* NQNameValue_next(NQNameValue* thiz, NQNameValueEntry* entry)
{
  if (entry->list.next == &thiz->list)
    return NULL;
  return NQ_CONTAINER_OF(entry->list.next, struct NQNameValueEntry, list);
}
