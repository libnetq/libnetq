/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_LOG_TAG "NQUrlPath"

#include "config.h"
#include "libnetq/UrlPath.h"

#include <libnetq/string/StringUtil.h>
#include <libnetq/Assert.h>
#include <libnetq/fs/Path.h>
#include <libnetq/Limits.h>
#include <libnetq/CType.h>
#include <libnetq/Malloc.h>
#include <libnetq/URL.h>
#include <libnetq/Log.h>

struct SegmentEntry {
  char* name;
  char* value;
};

NQ_STATIC_ASSERT(NQ_ALIGN_UP(sizeof(struct SegmentEntry), sizeof(void*)) == sizeof(struct SegmentEntry), "");

struct NQUrlPath {
  char* path;
  bool isAbsolute;
  uint16_t segmentCount;
  struct SegmentEntry segmentEntries[1];
};

static struct NQUrlPath s_rootPath = {
  .path = "/",
  .isAbsolute = true,
  .segmentCount = 0,
  .segmentEntries =
  {
    { .name = NULL, .value = NULL },
  },
};

struct PathInitInfo {
  bool isPatternMatch;
  size_t segmentCount;
  size_t pathLength;
  size_t namesLength;
  size_t valuesLength;
};

static NQ_ALWAYS_INLINE
size_t urlPathInit(NQUrlPath* thiz, const char* path, const char* pattern, struct PathInitInfo* pinfo, bool isInit)
{
  struct PathInitInfo info = {
    .isPatternMatch = true,
    .segmentCount = 0,
    .pathLength = 0,
    .namesLength = 0,
    .valuesLength = 0,
  };

  if (isInit) {
    char* ptr = (char*)thiz + sizeof(*thiz) + sizeof(struct SegmentEntry) * pinfo->segmentCount;
    thiz->path = ptr;
    ptr += pinfo->pathLength;
    thiz->segmentEntries[0].name = ptr;
    ptr += pinfo->namesLength;
    thiz->segmentEntries[0].value = ptr;
  }

  if (isInit) {
    thiz->isAbsolute = NQIsPathSeparator(path[0]);
  }

  size_t nameLength = 0;
  size_t valueLength = 0;

  while(true) {
    char ch = *path;

    if (NQIsPathSeparator(ch) || ch == '\0') {
      if (pattern) {
        if (*pattern == ch)
          pattern++;
        else
          pattern = NULL;
      }
      if (isInit) {
        thiz->path[info.pathLength] = ch;
      }
      info.pathLength++;
      if (valueLength != 0) {
        if (isInit) {
          struct SegmentEntry* entry = &thiz->segmentEntries[info.segmentCount];
          entry[1].name = entry->name + (nameLength ? nameLength + 1 : 0);
          entry[1].value = entry->value + valueLength + 1;
          entry->value[valueLength] = '\0';
          NQURLDecode(entry->value, valueLength, entry->value, valueLength);
          if (pattern && nameLength) {
            memcpy(entry->name, pattern - nameLength - 2, nameLength);
            entry->name[nameLength] = '\0';
          }
          else
            entry->name = NULL;
        }

        if (nameLength)
          info.namesLength += nameLength + 1;

        info.valuesLength++;
        info.segmentCount++;

        nameLength = 0;
        valueLength = 0;
      }
      if (ch == '\0') {
        break;
      }
    }
    else {
      if (pattern && nameLength == 0) {
        if (*pattern == ch)
          pattern++;
        else if (valueLength != 0 || *pattern != '{')
          pattern = NULL;
        else {
          char* nameEnd = strchr(pattern + 1, '}');
          if (!nameEnd)
            pattern = NULL;
          else {
            nameLength = (size_t)(nameEnd - pattern - 1);
            pattern = nameLength ? nameEnd + 1 : NULL;
          }
        }
      }
      if (isInit) {
        thiz->path[info.pathLength] = ch;
        thiz->segmentEntries[info.segmentCount].value[valueLength] = ch;
      }

      info.valuesLength++;
      info.pathLength++;

      valueLength++;
    }

    path++;
  }

  if (!pattern) {
    info.isPatternMatch = false;
    info.namesLength = 0;
  }

  size_t sizeInBytes = sizeof(*thiz) + sizeof(struct SegmentEntry) * info.segmentCount + info.pathLength + info.namesLength + info.valuesLength;

  if (isInit) {
    thiz->segmentCount = info.segmentCount;
    struct SegmentEntry* entry = &thiz->segmentEntries[info.segmentCount];
    NQ_ASSERT((char*)thiz + sizeInBytes == entry->value);
    entry->name = NULL;
    entry->value = NULL;
    NQ_ASSERT(info.segmentCount == pinfo->segmentCount && info.pathLength == pinfo->pathLength);
    NQ_ASSERT(info.namesLength == pinfo->namesLength && info.valuesLength == pinfo->valuesLength);
  }
  else {
    *pinfo = info;
  }

  return sizeInBytes;
}

NQUrlPath* NQUrlPath_create(const char* path, const char* pattern, bool onlymatch)
{
  if (path == NULL || *path == '\0')
    return NULL;

  if (NQIsPathSeparator(path[0]) && path[1] == '\0') {
    if (onlymatch && (path[0] != pattern[0] || path[1] != pattern[1]))
      return NULL;
    return &s_rootPath;
  }

  struct PathInitInfo info;
  size_t sizeInBytes = urlPathInit(NULL, path, pattern, &info, false);

  if (onlymatch && !info.isPatternMatch) {
    return NULL;
  }

  if (NQ_UINT16_MAX < info.pathLength) {
    NQ_LOGI("UrlPath has exceeded its " NQ_STRINGIZE(NQ_UINT16_MAX) " length");
    return 0;
  }

  NQUrlPath* thiz = (NQUrlPath*)NQMalloc(sizeInBytes);
  if (!thiz) {
    NQ_LOGI("No Memory");
    return NULL;
  }

  urlPathInit(thiz, path, info.isPatternMatch ? pattern : NULL, &info, true);
  return thiz;
}

void NQUrlPath_destroy(NQUrlPath* thiz)
{
  if (thiz != &s_rootPath)
    NQFree(thiz);
}

size_t NQUrlPath_segmentCount(const NQUrlPath* thiz)
{
  return thiz->segmentCount;
}

const char* NQUrlPath_segmentAt(const NQUrlPath* thiz, size_t index)
{
  return (index < thiz->segmentCount) ?  thiz->segmentEntries[index].value : NULL;
}

const char* NQUrlPath_segment(const NQUrlPath* thiz, const char* name)
{
  const struct SegmentEntry* entry = thiz->segmentEntries;
  while (entry->value) {
    if (entry->name && strcmp(entry->name, name) == 0)
      return entry->value;
    entry++;
  }
  return NULL;
}

const char* NQUrlPath_path(const NQUrlPath* thiz)
{
  return thiz->path;
}

bool NQUrlPath_isAbsolute(const NQUrlPath* thiz)
{
  return thiz->isAbsolute;
}

static bool isUrlPathChar(char ch)
{
  if (NQIsAlpha(ch))
    return true;
  else if (NQIsDigit(ch))
    return true;
  else if (ch == '-')
    return true;
  else if (ch == '_')
    return true;
  else if (ch == '/')
    return true;
  else if (ch == '.')
    return true;
  else if (ch == '~')
    return true;

  return false;
}

bool NQIsUrlPath(const char* path)
{
  char ch = *path++;
  if (ch != '\0') {
    for (;;) {
      if (!isUrlPathChar(ch))
        break;
      ch = *path++;
      if (ch == '\0')
        return true;
    }
  }

  return false;
}

bool NQIsUrlPathPattern(const char* pattern)
{
  if (pattern[0] == '\0')
    return false;

  enum {
    kEmptyState,
    kNameState,
    kValueState,
  };

  int state = kEmptyState;
  size_t namesCount = 0;

  while (true) {
    char ch = *pattern;
    if (ch == '\0') {
      if (state == kNameState)
        return false;
      break;
    }

    switch (state) {
    case kEmptyState:
      if (ch == '{')
        state = kNameState;
      else if (!NQIsPathSeparator(ch))
        state = kValueState;
      break;

    case kNameState:
      if (ch == '}') {
        state = kEmptyState;
        namesCount++;
      }
      break;

    case kValueState:
      if (NQIsPathSeparator(ch))
        state = kEmptyState;
      break;

    default:
      return false;
    }

    pattern++;
  }

  return !!namesCount;
}

bool NQUrlPathStartsWith(const char* path, const char* search)
{
  char lastChar = '\0';
  while (*search) {
    lastChar = *path;
    if (lastChar == '\0' || lastChar != *search)
      return false;
    path++;
    search++;
  }

  if (*path == '\0' || NQIsPathSeparator(*path))
    return true;

  return NQIsPathSeparator(lastChar);
}
