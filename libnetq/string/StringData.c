/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/string/StringData.h"

#include <libnetq/string/StringUtil.h>
#include <libnetq/Limits.h>
#include <libnetq/Malloc.h>
#include <libnetq/Log.h>

void NQStringData_init(NQStringData* thiz)
{
  thiz->characters = NQCStrEmpty();
  thiz->length = 0;
}

void NQStringData_finalize(NQStringData* thiz)
{
  if (NQCStrEmpty() != thiz->characters)
    NQFree((void*)thiz->characters);
}

bool NQStringData_set(NQStringData* thiz, const char* characters)
{
  return NQStringData_set2(thiz, characters, NQStrlen(characters));
}

bool NQStringData_set2(NQStringData* thiz, const char* characters, size_t length)
{
  if (length == 0) {
    NQStringData_finalize(thiz);
    NQStringData_init(thiz);
    return true;
  }

  if (length >= NQ_UINT32_MAX) {
    NQ_LOGE("String length exceeded");
    return false;
  }

  if (thiz->length == length) {
    NQMemcpy((char*)thiz->characters, characters, length);
    return true;
  }

  char* newCharacters = NQMalloc(length + 1);
  if (newCharacters == NULL) {
    return false;
  }

  NQMemcpy(newCharacters, characters, length);
  newCharacters[length] = '\0';

  NQStringData_finalize(thiz);

  thiz->characters = newCharacters;
  thiz->length = length;

  return true;
}

bool NQStringData_printf(NQStringData* thiz, const char* format, ...)
{
  bool result;
  va_list args;

  va_start(args, format);
  result = NQStringData_vprintf(thiz, format, args);
  va_end(args);

  return result;
}

bool NQStringData_vprintf(NQStringData* thiz, const char* format, va_list args)
{
  char* newCharacters = NQCStrFormatV(format, args);
  if (newCharacters == NULL) {
    return false;
  }

  NQStringData_finalize(thiz);

  thiz->characters = newCharacters;
  thiz->length = NQStrlen(newCharacters); // FIXME

  return true;
}
