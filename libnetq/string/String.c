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

#ifndef NQ_HAVE_ARCH_MEMCPY
void* NQMemcpy(void* dest, const void* src, size_t count)
{
  char* d = (char*)dest;
  const char* s = (const char*)src;
  while (count) {
    count--;
    *d++ = *s++;
  }
  return dest;
}
#endif

#ifndef NQ_HAVE_ARCH_MEMCMP
int NQMemcmp(const void* m1, const void* m2, size_t count)
{
  const unsigned char* p1 = (const unsigned char*)m1;
  const unsigned char* p2 = (const unsigned char*)m2;
  for (; count; count--, p1++, p2++) {
    if (*p1 != *p2)
      return *p1 - *p2;
  }
  return 0;
}
#endif

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
