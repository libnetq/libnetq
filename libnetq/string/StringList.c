/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_CLASS_NAME "NQStringList"
#define NQ_LOG_TAG NQ_CLASS_NAME

#include "config.h"
#include "libnetq/string/StringList.h"

#include <libnetq/String.h>
#include <libnetq/Malloc.h>
#include <libnetq/Limits.h>
#include <libnetq/Log.h>

struct NQStringListEntry {
  NQListHead list;
  uint32_t length;
  char characters[1];
};

#define listToEntry(list) (struct NQStringListEntry*)(list)

void NQStringList_finalize(NQStringList* thiz)
{
  NQListHead* iter = thiz->impl.next;
  while (iter != &thiz->impl) {
    struct NQStringListEntry* entry = listToEntry(iter);
    NQListHead* next = iter->next;
    NQFree(entry);
    iter = next;
  }
}

bool NQStringList_append(NQStringList* thiz, const char* characters)
{
  return NQStringList_append2(thiz, characters, NQStrlen(characters));
}

bool NQStringList_append2(NQStringList* thiz, const char* characters, size_t length)
{
  struct NQStringListEntry* entry;

  if (NQ_UINT32_MAX < length) {
    NQ_LOGE("Length exceeded %llu", (unsigned long long)length);
    return false;
  }

  entry = (struct NQStringListEntry*)NQMalloc(sizeof(*entry) + length);
  if (entry == NULL) {
    NQ_LOGE("No Memory");
    return false;
  }

  entry->length = (uint32_t)length;
  NQMemcpy(entry->characters, characters, length);
  entry->characters[length] = '\0';
  NQListHead_addBack(&thiz->impl, &entry->list);
  thiz->size++;

  return true;
}

bool NQStringList_split(NQStringList* thiz, const char* str, char separator)
{
  NQStringList stringList;
  NQStringList_init(&stringList);

  size_t len = 0;
  char* ptr = (char*)str;

  while (true) {
    char ch = ptr[len];
    if (ch == separator) {
      if (!NQStringList_append2(&stringList, ptr, len)) {
        NQStringList_finalize(&stringList);
        return false;
      }
      ptr += len + 1;
      len = 0;
    }
    else if (ch == '\0') {
      if (!NQStringList_append2(&stringList, ptr, len)) {
        NQStringList_finalize(&stringList);
        return false;
      }
      while (!NQStringList_isEmpty(&stringList)) {
        NQListHead* iter = stringList.impl.next;
        NQListHead_remove(iter);
        NQListHead_addBack(&thiz->impl, iter);
        thiz->size++;
      }
      break;
    }
    else {
      len++;
    }
  }

  NQStringList_finalize(&stringList);
  return true;
}

const NQStringListIter* NQStringList_firstIter(const NQStringList* thiz)
{
  return thiz->impl.next != &thiz->impl ? (const NQStringListIter*)thiz->impl.next : NULL;
}

const struct NQStringListIter* NQStringList_nextIter(const NQStringList* thiz, const NQStringListIter* iter)
{
  const struct NQStringListEntry* entry = listToEntry((const NQListHead*)iter);
  return entry->list.next != &thiz->impl ? (const NQStringListIter*)entry->list.next : NULL;
}

uint32_t NQStringListIter_length(const NQStringListIter* iter)
{
  const struct NQStringListEntry* entry = listToEntry((const NQListHead*)iter);
  return entry->length;
}

const char* NQStringListIter_characters(const NQStringListIter* iter)
{
  const struct NQStringListEntry* entry = listToEntry((const NQListHead*)iter);
  return entry->characters;
}
