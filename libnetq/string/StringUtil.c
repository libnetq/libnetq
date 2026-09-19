/*
 * MIT License
 *
 * Copyright (c) 2022-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/string/StringUtil.h"

#include <libnetq/Malloc.h>
#include <libnetq/string/Sprintf.h>
#include <libnetq/string/String.h>
#include <libnetq/Assert.h>

static const char s_empty[] = "";

const char* NQCStrEmpty(void)
{
  return s_empty;
}

const char* NQCStrEnsure(const char* str)
{
  return str ? str : s_empty;
}

void NQCStrFree(const char* str)
{
  NQFree((void*)str);
}

size_t NQCStrLen16(const uint16_t* s)
{
  const uint16_t* p = s;
  while (*p)
    p++;
  return p - s;
}

char* NQCStrDuplicate(const char* str)
{
  size_t lenz = NQStrlen(str) + 1;

  char* dup = (char*)NQMalloc(lenz);
  if (dup == NULL)
    return NULL;

  memcpy(dup, str, lenz);
  return dup;
}

char* NQCStrDuplicateWithLength(const char* str, size_t len)
{
  char* dup = (char*)NQMalloc(len + 1);
  if (dup == NULL)
    return NULL;

  memcpy(dup, str, len);
  dup[len] = '\0';
  return dup;
}

const char* NQCStrFindCStrn(const char* s1, const char* s2, size_t n)
{
  size_t sz = strlen(s1);
  while (sz < n) {
    if (!memcmp(s1, s2, sz))
      return s2;
    s2++;
    n--;
  }
  return NULL;
}

bool NQCStrStartsWith(const char* str, const char* search)
{
  while (*search) {
    if (*str == '\0' || *str != *search)
      return false;
    str++;
    search++;
  }

  return true;
}

char* NQCStrFormatV(const char* format, va_list args)
{
  NQ_ASSERT(format);

  int n;
  char* result;
  size_t size;
  va_list argsCopy;

  va_copy(argsCopy, args);

#ifdef NQ_COMPILER_MSVC
  n = _vscprintf(format, argsCopy);
#else
  char ch;
  n = vsnprintf(&ch, 1, format, argsCopy);
#endif

  va_end(argsCopy);

  if (n < 0)
    result = NULL;
  else if (n == 0)
    result = NQZalloc(1);
  else {
    size = (size_t)n + 1;
    result = (char*)NQMalloc(size);
    if (result != NULL) {
      va_copy(argsCopy, args);
      vsnprintf(result, size, format, argsCopy);
      va_end(argsCopy);
    }
  }

  return result;
}

char* NQCStrFormat(const char* format, ...)
{
  char* result;
  va_list args;

  va_start(args, format);
  result = NQCStrFormatV(format, args);
  va_end(args);

  return result;
}
