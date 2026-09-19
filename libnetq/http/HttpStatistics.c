/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/http/HttpStatistics.h"

#include <libnetq/string/String.h>
#include <libnetq/Malloc.h>
#include <libnetq/Limits.h>
#include <libnetq/List.h>
#include <libnetq/Hash.h>
#include <libnetq/Random.h>

#define kItemDataCount (1 << 6)

struct WebStatisticEntry {
  volatile uint32_t counter;
  NQSList slist;
  uint32_t ulen;
  uint32_t hash;
  char method[8];
  char url[1];
};

struct NQHttpStatistics {
  uint32_t mask;
  uint32_t count;
  NQSList entries[1];
};

NQHttpStatistics* NQHttpStatistics_create(void)
{
  NQHttpStatistics* thiz;

  thiz = (NQHttpStatistics*)NQMalloc(sizeof(*thiz) + sizeof(thiz->entries) * (kItemDataCount - 1));
  if (thiz == NULL)
    return NULL;

  thiz->mask = kItemDataCount - 1;
  thiz->count = 0;

  for (size_t i = 0; i < kItemDataCount; i++) {
    NQSList_init(&thiz->entries[i]);
  }

  return thiz;
}

void NQHttpStatistics_destroy(NQHttpStatistics* thiz)
{
  for (size_t i = 0; i < kItemDataCount; i++) {
    NQSList* iter = thiz->entries[i].next;
    while (iter != NULL) {
      struct WebStatisticEntry* entry = NQ_CONTAINER_OF(iter, struct WebStatisticEntry, slist);
      iter = entry->slist.next;
      NQFree(entry);
    }
  }
  NQFree(thiz);
}

bool NQHttpStatistics_add(NQHttpStatistics* thiz, const char* method, const char* url)
{
  struct WebStatisticEntry* newEntry;
  size_t mlen = NQStrlen(method);
  if (mlen == 0 || mlen >= sizeof(newEntry->method))
    return false;

  size_t ulen = NQStrlen(url);
  if (ulen == 0 || NQ_UINT32_MAX < ulen)
    return false;

  newEntry = (struct WebStatisticEntry*)NQMalloc(sizeof(*newEntry) + ulen);
  if (newEntry == NULL)
    return false;

  newEntry->counter = 0;
  memcpy(newEntry->method, method, mlen + 1);
  newEntry->ulen = (uint32_t)ulen;
  memcpy(newEntry->url, url, ulen + 1);
  newEntry->hash = NQHashString(url, ulen);
  uint32_t index = newEntry->hash & thiz->mask;

  NQSList* iter = thiz->entries[index].next;
  NQSList* prev = &newEntry->slist;
  newEntry->slist.next = iter;
  thiz->entries[index].next = prev;

  while (iter != NULL) {
    struct WebStatisticEntry* entry = NQ_CONTAINER_OF(iter, struct WebStatisticEntry, slist);
    if ((entry->hash == newEntry->hash) && (entry->ulen == newEntry->ulen)) {
      if (memcmp(entry->method, method, mlen + 1) == 0 && memcmp(entry->url, url, ulen) == 0) {
        prev->next = iter->next;
        NQFree(entry);
        return true;
      }
    }
    prev = &entry->slist;
    iter = entry->slist.next;
  }

  thiz->count++;
  return true;
}

bool NQHttpStatistics_inc(NQHttpStatistics* thiz, const char* method, const char* url)
{
  struct WebStatisticEntry* entry;
  size_t mlen = NQStrlen(method);
  if (mlen == 0 || sizeof(entry->method) < mlen)
    return false;

  size_t ulen = NQStrlen(url);
  if (ulen == 0)
    return false;

  uint32_t hash = NQHashString(url, ulen);
  uint32_t index = hash & thiz->mask;

  NQSList* iter = thiz->entries[index].next;
  while (iter != NULL) {
    entry = NQ_CONTAINER_OF(iter, struct WebStatisticEntry, slist);
    if (entry->hash == hash && entry->ulen == ulen) {
      if (memcmp(entry->method, method, mlen) == 0 && memcmp(entry->url, url, ulen) == 0) {
        entry->counter++;
        return true;
      }
    }
    iter = iter->next;
  }

  return false;
}

bool NQHttpStatistics_writeTo(NQHttpStatistics* thiz, NQJSONWriter* writer)
{
  if (!NQJSONWriter_writeArrayBegin(writer))
    return false;

  for (size_t i = 0; i < kItemDataCount; i++) {
    NQSList* iter = thiz->entries[i].next;
    while (iter != NULL) {
      struct WebStatisticEntry* entry = NQ_CONTAINER_OF(iter, struct WebStatisticEntry, slist);
      if (!NQJSONWriter_writeObjectBegin(writer))
        return false;

      if (!NQJSONWriter_writeKeyString(writer, "url", entry->url))
        return false;

      if (!NQJSONWriter_writeKeyString(writer, "method", entry->method))
        return false;

      if (!NQJSONWriter_writeKeyUint32(writer, "counter", entry->counter))
        return false;

      if (!NQJSONWriter_writeObjectEnd(writer))
        return false;
      iter = entry->slist.next;
    }
  }

  if (!NQJSONWriter_writeArrayEnd(writer))
    return false;

  return true;
}
