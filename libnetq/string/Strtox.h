/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_STRING_STRTOX_H
#define _LIBNETQ_STRING_STRTOX_H

#include <libnetq/Basic.h>

#ifdef NQ_OS_KERNEL
#include <linux/kstrtox.h>
#else
#include <stdlib.h>
#endif

static inline long NQSimpleStrtol(const char* str, char** endstr, unsigned base)
{
#ifdef NQ_OS_KERNEL
  return simple_strtol(str, endstr, base);
#else
  return strtol(str, endstr, (int)base);
#endif
}

static inline unsigned long NQSimpleStrtoul(const char* str, char** endstr, unsigned base)
{
#ifdef NQ_OS_KERNEL
  return simple_strtoul(str, endstr, base);
#else
  return strtoul(str, endstr, (int)base);
#endif
}

static inline long long NQSimpleStrtoll(const char* str, char** endstr, unsigned base)
{
#ifdef NQ_OS_KERNEL
  return simple_strtoll(str, endstr, base);
#else
  return strtoll(str, endstr, (int)base);
#endif
}

static inline unsigned long long NQSimpleStrtoull(const char* str, char** endstr, unsigned base)
{
#ifdef NQ_OS_KERNEL
  return simple_strtoull(str, endstr, base);
#else
  return strtoull(str, endstr, (int)base);
#endif
}

#endif /* _LIBNETQ_STRING_STRTOX_H */
