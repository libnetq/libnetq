/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_STRING_STRING_H
#define _LIBNETQ_STRING_STRING_H

#include <libnetq/Basic.h>

#if defined(NQ_OS_KERNEL)
#include <linux/string.h>
#elif defined(NQ_OS_WINDOWS) || defined(NQ_OS_UNIX) || defined(NQCONFIG_USE_STRING_H)
#include <string.h>
#endif

#if defined(NQ_OS_KERNEL) || defined(NQ_OS_WINDOWS) || defined(NQ_OS_UNIX) || defined(NQCONFIG_USE_STRING_H)
#define NQ_HAVE_ARCH_MEMCPY 1
#define NQ_HAVE_ARCH_MEMSET 1
#define NQ_HAVE_ARCH_MEMCMP 1
#define NQ_HAVE_ARCH_MEMCHR 1
#define NQ_HAVE_ARCH_MEMMOVE 1
#define NQ_HAVE_ARCH_STRLEN 1
#define NQ_HAVE_ARCH_STRRCHR 1
#define NQ_HAVE_ARCH_STRCMP 1
#define NQ_HAVE_ARCH_STRNCMP 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NQ_HAVE_ARCH_MEMCPY
NQ_EXPORT void* NQMemcpy(void* dest, const void* src, size_t count);
#elif NQ_HAS_BUILTIN(__builtin_memcpy)
#define NQMemcpy __builtin_memcpy
#else
#define NQMemcpy memcpy
#endif

#ifndef NQ_HAVE_ARCH_MEMSET
NQ_EXPORT void* NQMemset(void* mem, int ch, size_t count);
#elif NQ_HAS_BUILTIN(__builtin_memset)
#define NQMemset __builtin_memset
#else
#define NQMemset memset
#endif

#ifndef NQ_HAVE_ARCH_MEMCMP
NQ_EXPORT int NQMemcmp(const void* m1, const void* m2, size_t count);
#else
#define NQMemcmp memcmp
#endif

#ifndef NQ_HAVE_ARCH_MEMCHR
NQ_EXPORT void* NQMemchr(const void* mem, int ch, size_t count);
#else
#define NQMemchr memchr
#endif

#ifndef NQ_HAVE_ARCH_MEMMOVE
NQ_EXPORT void* NQMemmove(void* dest, const void* src, size_t count);
#else
#define NQMemmove memmove
#endif

#if defined(NQ_COMPILER_MSVC)
#define NQStrcasecmp _stricmp
#define NQStrncasecmp _strnicmp
#else
#define NQStrcasecmp strcasecmp
#define NQStrncasecmp strncasecmp
#endif

#ifndef NQ_HAVE_ARCH_STRLEN
NQ_EXPORT size_t NQStrlen(const char* s);
#else
#define NQStrlen strlen
#endif

#ifndef NQ_HAVE_ARCH_STRRCHR
NQ_EXPORT char* NQStrrchr(const char* s, int c);
#else
#define NQStrrchr strrchr
#endif

#ifndef NQ_HAVE_ARCH_STRCMP
NQ_EXPORT int NQStrcmp(const char* s1, const char* s2);
#else
#define NQStrcmp strcmp
#endif

#ifndef NQ_HAVE_ARCH_STRNCMP
NQ_EXPORT int NQStrncmp(const char* s1, const char* s2, size_t count);
#else
#define NQStrncmp strncmp
#endif

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_STRING_STRING_H */
