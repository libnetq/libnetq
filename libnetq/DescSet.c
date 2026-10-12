/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_CLASS_NAME "NQDescSet"
#define NQ_LOG_TAG NQ_CLASS_NAME

#include "config.h"
#include "libnetq/DescSet.h"

#include <libnetq/String.h>
#include <libnetq/Random.h>
#include <libnetq/Malloc.h>
#include <libnetq/Limits.h>
#include <libnetq/Assert.h>

struct NQDescEntry {
  NQDescEntry* next;
  NQDescEntry* prev;
  uint32_t flags;
  NQDescIdentifier id;
};

struct NQDescList {
  NQDescEntry* first;
  NQDescEntry* last;
  size_t size;
};

struct NQDescSet {
  NQDescIdentifier maxId;
  size_t maxCount;
  size_t entrySize;
  struct NQDescList allocList;
  struct NQDescList freeList;
};

#if NQ_ASSERT_ENABLED
#define NQ_DESC_FILL_CHAR 0xAA
#else
#define NQ_DESC_FILL_CHAR 0
#endif

#define NQ_DESC_FREE_FLAG (1 << 0)

NQ_STATIC_ASSERT(sizeof(NQDescSet) % sizeof(void*) == 0, "Invalid size of NQDescSet class");
NQ_STATIC_ASSERT(sizeof(NQDescEntry) % sizeof(void*) == 0, "Invalid size of NQDescEntry class");

#if NQ_DESC_FILL_CHAR == 0
#define NQDescEntry_checkData(...)
#else
static bool NQIsFillMemory(const void* mem, int value, size_t size)
{
  size_t index;
  
  for (index = 0; index < size; index++) {
    if (((uint8_t*)mem)[index] != (value & 0xFF))
      return false;
  }

  return true;
}
static void NQDescEntry_checkData(NQDescEntry* entry, size_t entrySize)
{
  void* data = NQDescEntry_data(entry);
  size_t size = entrySize - sizeof(NQDescEntry);
  NQ_ASSERT(NQIsFillMemory(data, NQ_DESC_FILL_CHAR, size));
}
#endif

static NQDescIdentifier makeIdentifier(NQDescIdentifier id, NQDescIdentifier maxId)
{
  NQDescIdentifier result;
  
  size_t index = NQDescIdentifierToIndex(id);

  do {
    NQGetUnlimitedRandom((uint8_t*)&result, sizeof(result));
    if (maxId != NQ_UINTPTR_MAX)
      result %= (maxId + 1);
    result &= ~(NQ_DESC_NUM_MAX - 1);
    if (result == NQ_DESC_INVALID)
      continue;
    result |= index;
  } while (id == result);

  return result;
}

static NQDescEntry* NQDescList_remove(struct NQDescList* list, NQDescEntry* entry)
{
  if (entry->next != NULL)
    entry->next->prev = entry->prev;
  else {
    NQ_ASSERT(list->last == entry);
    list->last = entry->prev;
  }

  if (entry->prev != NULL)
    entry->prev->next = entry->next;
  else {
    NQ_ASSERT(list->first == entry);
    list->first = entry->next;
  }

  list->size--;

  return entry;
}

static void NQDescList_push(struct NQDescList* list, NQDescEntry* entry)
{
  entry->prev = list->last;

  if (list->last != NULL)
    list->last->next = entry;
  else {
    NQ_ASSERT(list->size == 0);
    NQ_ASSERT(list->first == NULL);
    list->first = entry;
  }

  entry->next = NULL;
  list->last = entry;
  list->size++;
}

static NQDescEntry* NQDescList_shift(struct NQDescList* list)
{
  if (list->first == NULL)
    return NULL;

  NQDescEntry* entry = list->first;

  list->first = entry->next;

  if (list->first != NULL)
    list->first->prev = NULL;
  else
    list->last = NULL;

  list->size--;

  return entry;
}

static void NQDescEntry_fill(NQDescEntry* entry, int value, size_t size)
{
  void* data = NQDescEntry_data(entry);
  NQMemset(data, value, size);
}

static NQDescEntry* NQDescSet_getAllEntries(NQDescSet* descset, size_t index)
{
  uint8_t* ptr = (uint8_t*)(descset + 1);
  ptr += descset->entrySize * index;
  NQ_ASSERT(((uintptr_t)ptr & (sizeof(void*) - 1)) == 0);
  return (NQDescEntry*)ptr;
}

static void NQDescList_init(struct NQDescList* list)
{
  list->first = NULL;
  list->last = NULL;
  list->size = 0;
}

NQDescSet* NQDescSet_create(size_t num, size_t size)
{
  size_t n;

  if (NQ_DESC_NUM_MAX < num || size == 0)
    return NULL;

  size_t entrySize = NQ_ALIGN_UP(sizeof(NQDescEntry) + size, sizeof(void*));
  size_t allocSize = sizeof(NQDescSet) + num * entrySize;

  NQDescSet* descset = (NQDescSet*)NQMalloc(allocSize);
  if (descset == NULL)
    return NULL;

  descset->maxId = NQ_UINTPTR_MAX;
  descset->maxCount = num;
  descset->entrySize = entrySize;
  NQDescList_init(&descset->allocList);
  NQDescList_init(&descset->freeList);

  for (n = 0; n < num; n++) {
    NQDescEntry* entry = NQDescSet_getAllEntries(descset, n);
    entry->id = (NQDescIdentifier)n;
    entry->flags = NQ_DESC_FREE_FLAG;

#if NQ_DESC_FILL_CHAR != 0
    NQDescEntry_fill(entry, NQ_DESC_FILL_CHAR, descset->entrySize - sizeof(NQDescEntry));
#endif

    NQDescList_push(&descset->freeList, entry);
  }

  return descset;
}

void NQDescSet_destroy(NQDescSet* descset)
{
  NQFree((void*)descset);
}

bool NQDescSet_setMaxId(NQDescSet* descset, NQDescIdentifier id)
{
  if (id < NQ_DESC_NUM_MAX)
    return false;

  descset->maxId = id;
  return true;
}

size_t NQDescSet_size(const NQDescSet* descset)
{
  return descset->allocList.size;
}

bool NQDescSet_isEmpty(const NQDescSet* descset)
{
  return descset->allocList.size == 0;
}

NQDescIdentifier NQDescSet_allocEntry(NQDescSet* descset, void** ptr)
{
  NQDescEntry* entry = NQDescList_shift(&descset->freeList);
  if (entry == NULL) {
    if (ptr)
      *ptr = NULL;
    return  NQ_DESC_INVALID;
  }

  NQDescEntry_checkData(entry, descset->entrySize);
  NQ_ASSERT(entry->flags & NQ_DESC_FREE_FLAG);

  entry->id = makeIdentifier(entry->id, descset->maxId);
  entry->flags &= ~NQ_DESC_FREE_FLAG;
  NQDescList_push(&descset->allocList, entry);

  if (ptr)
    *ptr = NQDescEntry_data(entry);

  return entry->id;
}

static inline NQDescEntry* NQDescSet_findEntry(NQDescSet* descset, NQDescIdentifier id)
{
  size_t index = NQDescIdentifierToIndex(id);
  NQDescEntry* entry = NQDescSet_getAllEntries(descset, index);

  if (entry->id != id)
    return NULL;

  return entry;
}

void NQDescSet_releaseEntry(NQDescSet* descset, NQDescIdentifier id)
{
  NQDescEntry* entry = NQDescSet_findEntry(descset, id);
  if (entry == NULL)
    return;

  if (entry->flags & NQ_DESC_FREE_FLAG)
    return;

  NQDescList_remove(&descset->allocList, entry);
  entry->flags |= NQ_DESC_FREE_FLAG;

#if NQ_DESC_FILL_CHAR != 0
  NQDescEntry_fill(entry, NQ_DESC_FILL_CHAR, descset->entrySize - sizeof(NQDescEntry));
#endif

  NQDescList_push(&descset->freeList, entry);
}

void* NQDescSet_findData(NQDescSet* descset, NQDescIdentifier id)
{
  NQDescEntry* entry = NQDescSet_findEntry(descset, id);
  if (entry == NULL || (entry->flags & NQ_DESC_FREE_FLAG))
    return NULL;

  return NQDescEntry_data(entry);
}

bool NQDescSet_hasEntry(NQDescSet* descset, NQDescIdentifier id)
{
  NQDescEntry* entry = NQDescSet_findEntry(descset, id);
  if (entry == NULL || (entry->flags & NQ_DESC_FREE_FLAG))
    return false;
  return true;
}

NQDescEntry* NQDescSet_first(NQDescSet* descset)
{
  return descset->allocList.first;
}

NQDescEntry* NQDescEntry_next(NQDescEntry* entry)
{
  return entry->next;
}

void* NQDescEntry_data(NQDescEntry* entry)
{
  return (entry + 1);
}
