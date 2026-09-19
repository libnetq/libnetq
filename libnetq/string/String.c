/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/string/String.h"

#include <libnetq/Malloc.h>
#include <libnetq/string/Sprintf.h>
#include <libnetq/Assert.h>

#ifndef NQ_HAVE_ARCH_STRLEN
size_t NQStrlen(const char* s)
{
  const char* p = s;
  while (*p)
    p++;
  return p - s;
}
#endif

#ifndef NQ_HAVE_ARCH_STRRCHR
char* NQStrrchr(const char* s, int c)
{
  const char* res = NULL;
  const char* p = s;
  while (*p) {
    if (*p++ == c) {
      res = p;
    }
  }
  return (char*)res;
}
#endif
