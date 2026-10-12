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

#include <libnetq/Assert.h>

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

#ifndef NQ_HAVE_ARCH_MEMSET
void* NQMemset(void* mem, int ch, size_t count)
{
  char* str = (char*)mem;
  while (count) {
    count--;
    *str++ = ch;
  }
  return mem;
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

#ifndef NQ_HAVE_ARCH_MEMCHR
void* NQMemchr(const void* mem, int ch, size_t count)
{
  NQ_UNUSED_PARAM(mem);
  NQ_UNUSED_PARAM(ch);
  NQ_UNUSED_PARAM(count);
  NQ_ASSERT_NOT_REACHED();
  return NULL;
}
#endif

#ifndef NQ_HAVE_ARCH_MEMMOVE
void* NQMemmove(void* dest, const void* src, size_t count)
{
  NQ_UNUSED_PARAM(dest);
  NQ_UNUSED_PARAM(src);
  NQ_UNUSED_PARAM(count);
  NQ_ASSERT_NOT_REACHED();
  return NULL;
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

#ifndef NQ_HAVE_ARCH_STRCHR
char* NQStrchr(const char* s, int c)
{
  const char* p = s;
  for (;;) {
    if (*p == (char)c)
      return (char*)p;
    if (*p == '\0')
      return NULL;
    p++;
  }
}
#endif

#ifndef NQ_HAVE_ARCH_STRRCHR
char* NQStrrchr(const char* s, int c)
{
  const char* res = NULL;
  const char* p = s;
  for (;;) {
    if (*p == (char)c)
      res = p;
    if (*p == '\0')
      break;
    p++;
  }
  return (char*)res;
}
#endif

#ifndef NQ_HAVE_ARCH_STRSTR
char* NQStrstr(const char* haystack, const char* needle)
{
  if (*needle == '\0')
    return (char*)haystack;
  for (; *haystack; haystack++) {
    const char* h = haystack;
    const char* n = needle;
    while (*n && *h == *n) {
      h++;
      n++;
    }
    if (*n == '\0')
      return (char*)haystack;
  }
  return NULL;
}
#endif

#ifndef NQ_HAVE_ARCH_STRCMP
int NQStrcmp(const char* s1, const char* s2)
{
  NQ_UNUSED_PARAM(s1);
  NQ_UNUSED_PARAM(s2);
  NQ_ASSERT_NOT_REACHED();
  return -1;
}
#endif

#ifndef NQ_HAVE_ARCH_STRNCMP
int NQStrncmp(const char* s1, const char* s2, size_t count)
{
  NQ_UNUSED_PARAM(s1);
  NQ_UNUSED_PARAM(s2);
  NQ_UNUSED_PARAM(count);
  NQ_ASSERT_NOT_REACHED();
  return -1;
}
#endif
