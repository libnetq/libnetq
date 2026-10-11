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

#if defined(NQ_OS_WINDOWS) || defined(NQ_OS_UNIX) || defined(NQCONFIG_USE_STDLIB_H)
#include <stdlib.h>
#elif defined(NQ_OS_KERNEL)
#include <linux/kstrtox.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(NQ_OS_WINDOWS) || defined(NQ_OS_UNIX) || defined(NQCONFIG_USE_STDLIB_H)
static inline long NQSimpleStrtol(const char* str, char** endstr, unsigned base)
{
  return strtol(str, endstr, (int)base);
}

static inline unsigned long NQSimpleStrtoul(const char* str, char** endstr, unsigned base)
{
  return strtoul(str, endstr, (int)base);
}

static inline long long NQSimpleStrtoll(const char* str, char** endstr, unsigned base)
{
  return strtoll(str, endstr, (int)base);
}

static inline unsigned long long NQSimpleStrtoull(const char* str, char** endstr, unsigned base)
{
  return strtoull(str, endstr, (int)base);
}
#elif defined(NQ_OS_KERNEL)
#define NQSimpleStrtol simple_strtol
#define NQSimpleStrtoul simple_strtoul
#define NQSimpleStrtoll simple_strtoll
#define NQSimpleStrtoull simple_strtoull
#else
NQ_EXPORT long NQSimpleStrtol(const char* str, char** endstr, unsigned base);
NQ_EXPORT unsigned long NQSimpleStrtoul(const char* str, char** endstr, unsigned base);
NQ_EXPORT long long NQSimpleStrtoll(const char* str, char** endstr, unsigned base);
NQ_EXPORT unsigned long long NQSimpleStrtoull(const char* str, char** endstr, unsigned base);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_STRING_STRTOX_H */
