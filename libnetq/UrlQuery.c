/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_LOG_TAG "NQUrlQuery"

#include "config.h"
#include "libnetq/UrlQuery.h"

#include <libnetq/string/StringUtil.h>
#include <libnetq/Assert.h>
#include <libnetq/fs/Path.h>
#include <libnetq/Limits.h>
#include <libnetq/CType.h>
#include <libnetq/Malloc.h>
#include <libnetq/URL.h>
#include <libnetq/Log.h>

struct ParamEntry {
  char* name;
  char* value;
};

NQ_STATIC_ASSERT(NQ_ALIGN_UP(sizeof(struct ParamEntry), sizeof(void*)) == sizeof(struct ParamEntry), "");

struct NQUrlQuery {
  char* query;
  uint16_t paramCount;
  struct ParamEntry paramEntries[1];
};

static struct NQUrlQuery s_emptyQuery = {
  .query = "?",
  .paramCount = 0,
  .paramEntries =
  {
    { .name = NULL, .value = NULL },
  },
};

struct QueryInitInfo {
  size_t paramCount;
  size_t queryLength;
  size_t namesLength;
  size_t valuesLength;
};

static NQ_ALWAYS_INLINE
size_t urlQueryInit(NQUrlQuery* thiz, const char* query, struct QueryInitInfo* pinfo, bool isInit)
{
  struct QueryInitInfo info = {
    .paramCount = 0,
    .queryLength = 1, // For '?'
    .namesLength = 0,
    .valuesLength = 0,
  };

  if (isInit) {
    char* ptr = (char*)thiz + sizeof(*thiz) + sizeof(struct ParamEntry) * pinfo->paramCount;
    thiz->query = ptr;
    ptr += pinfo->queryLength;
    thiz->paramEntries[0].name = ptr;
    ptr += pinfo->namesLength;
    thiz->paramEntries[0].value = ptr;
    thiz->query[0] = '?';
  }

  bool isValue = false;
  size_t nameLength = 0;
  size_t valueLength = 0;

  while(true) {
    char ch = *query;
    if (isInit) {
      thiz->query[info.queryLength] = ch;
    }
    info.queryLength++;

    if (ch == '&' || ch == '\0') {
      if (isInit) {
        struct ParamEntry* entry = &thiz->paramEntries[info.paramCount];
        entry[1].name = entry->name + nameLength + 1;
        entry[1].value = entry->value + valueLength + 1;
        entry->name[nameLength] = '\0';
        entry->value[valueLength] = '\0';
        if (nameLength)
          NQURLDecode(entry->name, nameLength, entry->name, nameLength);
        if (valueLength)
          NQURLDecode(entry->value, valueLength, entry->value, valueLength);
      }

      info.namesLength++;
      info.valuesLength++;
      info.paramCount++;

      if (ch == '\0') {
        break;
      }

      nameLength = 0;
      valueLength = 0;
      isValue = false;
    }
    else if (isValue) {
      if (isInit) {
        thiz->paramEntries[info.paramCount].value[valueLength] = ch;
      }

      info.valuesLength++;
      valueLength++;
    }
    else if (ch == '=') {
      isValue = true;
    }
    else {
      if (isInit) {
        thiz->paramEntries[info.paramCount].name[nameLength] = ch;
      }
      info.namesLength++;
      nameLength++;
    }

    query++;
  }

  size_t sizeInBytes = sizeof(*thiz) + sizeof(struct ParamEntry) * info.paramCount + info.queryLength + info.namesLength + info.valuesLength;

  if (isInit) {
    thiz->paramCount = (uint16_t)info.paramCount;
    struct ParamEntry* entry = &thiz->paramEntries[info.paramCount];
    NQ_ASSERT((char*)thiz + sizeInBytes == entry->value);
    entry->name = NULL;
    entry->value = NULL;
    NQ_ASSERT(info.paramCount == pinfo->paramCount && info.queryLength == pinfo->queryLength);
    NQ_ASSERT(info.namesLength == pinfo->namesLength && info.valuesLength == pinfo->valuesLength);
  }
  else {
    *pinfo = info;
  }

  return sizeInBytes;
}

NQUrlQuery* NQUrlQuery_create(const char* query)
{
  if (query == NULL || *query != '?')
    return NULL;

  if (*(++query) == '\0')
    return &s_emptyQuery;

  struct QueryInitInfo info;
  size_t sizeInBytes = urlQueryInit(NULL, query, &info, false);

  if (NQ_UINT16_MAX < info.queryLength) {
    NQ_LOGI("UrlQuery has exceeded its " NQ_STRINGIZE(NQ_UINT16_MAX) " length");
    return 0;
  }

  NQUrlQuery* thiz = (NQUrlQuery*)NQMalloc(sizeInBytes);
  if (!thiz) {
    NQ_LOGI("No Memory");
    return NULL;
  }

  urlQueryInit(thiz, query, &info, true);
  return thiz;
}

void NQUrlQuery_destroy(NQUrlQuery* thiz)
{
  if (thiz != &s_emptyQuery)
    NQFree(thiz);
}

size_t NQUrlQuery_count(const NQUrlQuery* thiz)
{
  return thiz->paramCount;
}

const char* NQUrlQuery_nameAt(const NQUrlQuery* thiz, size_t index)
{
  return (index < thiz->paramCount) ?  thiz->paramEntries[index].name : NULL;
}

const char* NQUrlQuery_valueAt(const NQUrlQuery* thiz, size_t index)
{
  return (index < thiz->paramCount) ?  thiz->paramEntries[index].value : NULL;
}

const char* NQUrlQuery_value(const NQUrlQuery* thiz, const char* name)
{
  const struct ParamEntry* entry = thiz->paramEntries;
  while (entry->value) {
    if (entry->name && strcmp(entry->name, name) == 0)
      return entry->value;
    entry++;
  }
  return NULL;
}

const char* NQUrlQuery_query(const NQUrlQuery* thiz)
{
  return thiz->query;
}
