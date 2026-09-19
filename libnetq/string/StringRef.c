/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_CLASS_NAME "NQStringRef"
#define NQ_LOG_TAG NQ_CLASS_NAME

#include "config.h"
#include "libnetq/string/StringRef.h"

#include <libnetq/string/Sprintf.h>
#include <libnetq/Compiler.h>
#include <libnetq/Malloc.h>
#include <libnetq/Limits.h>
#include <libnetq/Assert.h>
#include <libnetq/fs/FileHandle.h>
#include <libnetq/string/String.h>
#include <libnetq/Log.h>

#define NQ_STRING_REF_COUNT_STATIC 0x80000000

static NQString s_emptyStringStatic = {
  { NQ_STRING_REF_COUNT_STATIC }, 0, 0, { '\0' },
};

static inline void NQString_init(NQString* s, size_t length)
{
  NQ_ASSERT(length < NQ_UINT32_MAX);
  NQRefCount_init(&s->refCount);
  s->length = (uint32_t)length;
  s->flags = 0;
}

NQString* NQString_alloc(size_t length)
{
  if (NQ_UINT32_MAX < length) {
    NQ_LOGE("Not support large string");
    return NULL;
  }

  NQString* thiz = (NQString*)NQMalloc(sizeof(NQString) + length);
  NQString_init(thiz, length);
  return thiz;
}

NQString* NQString_create(const char* characters)
{
  size_t length = NQStrlen(characters);
  NQString* thiz = NQString_alloc(length);
  if (thiz == NULL)
    return NULL;

  memcpy(thiz->characters, characters, length + 1);
  return thiz;
}

NQString* NQString_create2(const char* characters, size_t length)
{
  NQString* thiz = NQString_alloc(length);
  if (thiz == NULL)
    return NULL;

  memcpy(thiz->characters, characters, length);
  thiz->characters[length] = '\0';
  return thiz;
}

NQString* NQString_format(const char* format, ...)
{
  NQ_ASSERT(format);

  int ret;
  NQString* s;
  uint32_t length;
  va_list args;

  va_start(args, format);

#ifdef NQ_COMPILER_MSVC
  ret = _vscprintf(format, args);
#else
  char ch;
  ret = vsnprintf(&ch, 1, format, args);
  va_end(args);
  va_start(args, format);
#endif
  
  if (ret == 0 || ret < 0)
    s = &s_emptyStringStatic;
  else {
    length = (size_t)ret;
    s = NQString_alloc(length);
    if (s != NULL)
      vsnprintf((char*)s->characters, length + 1, format, args);
  }

  va_end(args);

  return s;
}

NQString* NQString_retain(NQString* s)
{
  NQRefCount_ref(&s->refCount);
  return s;
}

void NQString_release(NQString* s)
{
  NQRefCount_unref(&s->refCount, NQFree, s);
}

NQString* NQString_fromFile(const char* filename)
{
  NQFileHandle handle;
  int ret = NQFileOpen(filename, NQ_FOPEN_READ, &handle);
  if (ret != 0) {
    NQ_LOGE("File '%s' returned error - %i", filename, -ret);
    return NULL;
  }

  long long size = NQFileGetSize(handle);
  if (size < 0) {
    NQ_LOGE("Error %i occurred while retrieving the size", (int)size);
    NQFileClose(handle);
    return NULL;
  }

  if (size > NQ_UINT32_MAX) {
    NQ_LOGE("File %s is too big", filename);
    NQFileClose(handle);
    return NULL;
  }

  NQString* thiz = NQString_alloc((uint32_t)size);
  if (thiz != NULL) {
    int64_t n = NQFileReadn(handle, (uint8_t*)NQString_characters(thiz), (int64_t)size);
    if (n == (int64_t)size)
      thiz->characters[size] = '\0';
    else {
      NQString_release(thiz);
      thiz = NULL;
    }
  }

  NQFileClose(handle);
  return thiz;
}
