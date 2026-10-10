/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_STRING_SPRINTF_H
#define _LIBNETQ_STRING_SPRINTF_H

#include <libnetq/Basic.h>

#if defined(NQ_OS_WINDOWS) || defined(NQ_OS_UNIX) || defined(NQCONFIG_USE_STDIO_H)
#include <stdio.h>
#elif defined(NQ_OS_KERNEL)
#include <linux/stdarg.h>
#include <linux/sprintf.h>
#else
#include <libnetq/VA.h>
#endif

#if defined(NQ_OS_KERNEL) || defined(NQ_OS_WINDOWS) || defined(NQ_OS_UNIX) || defined(NQCONFIG_USE_STDIO_H)
#define NQ_HAVE_ARCH_SPRINTF 1
#define NQ_HAVE_ARCH_VSPRINTF 1
#define NQ_HAVE_ARCH_SNPRINTF 1
#define NQ_HAVE_ARCH_VSNPRINTF 1
#endif

#ifndef NQ_HAVE_ARCH_VSPRINTF
NQ_EXPORT int NQSprintf(char* buf, const char* format, ...) NQ_ATTRIBUTE_PRINTF(2, 3);
#else
#define NQSprintf sprintf
#endif

#ifndef NQ_HAVE_ARCH_VSPRINTF
NQ_EXPORT int NQVsprintf(char* buf, const char* format, va_list args) NQ_ATTRIBUTE_PRINTF(2, 0);
#else
#define NQVsprintf vsprintf
#endif

#ifndef NQ_HAVE_ARCH_VSNPRINTF
NQ_EXPORT int NQSnprintf(char* buf, size_t len, const char* format, ...) NQ_ATTRIBUTE_PRINTF(3, 4);
#else
#define NQSnprintf snprintf
#endif

#ifndef NQ_HAVE_ARCH_VSNPRINTF
NQ_EXPORT int NQVsnprintf(char* buf, size_t len, const char* format, va_list args) NQ_ATTRIBUTE_PRINTF(3, 0);
#else
#define NQVsnprintf vsnprintf
#endif

#endif /* _LIBNETQ_STRING_SPRINTF_H */

