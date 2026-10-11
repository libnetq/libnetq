/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_TYPES_H
#define _LIBNETQ_TYPES_H

#include <libnetq/OS.h>
#include <libnetq/ConstExpr.h>

#ifdef NQ_OS_KERNEL
# include <linux/types.h>
# include <linux/stddef.h>
#elif defined(NQ_OS_WINDOWS) || defined(NQ_OS_UNIX)
# include <stdbool.h>
# include <stddef.h>
# include <stdint.h>
# include <stdlib.h>
#else

# if !defined(__SSIZE_T__) && defined(NQ_OS_WINDOWS)
#  if __SIZEOF_POINTER__ == __SIZEOF_INT__
#   define __SSIZE_T__ int
#  endif
#endif

# ifndef __SSIZE_T__
#  if __SIZEOF_POINTER__ == __SIZEOF_LONG__
#   define __SSIZE_T__ long
#  elif __SIZEOF_POINTER__ == __SIZEOF_LONG_LONG__
#   define __SSIZE_T__ long long
#  else
#   error Unknown pointer size
#  endif
 #endif

typedef _Bool bool;

typedef signed char int8_t;
typedef unsigned char uint8_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef int int32_t;
typedef unsigned uint32_t;

#if __SIZEOF_LONG__ == 8
typedef long int64_t;
typedef unsigned long uint64_t;
#else
typedef long long int64_t;
typedef unsigned long long uint64_t;

typedef __SSIZE_T__ ssize_t;
typedef unsigned __SSIZE_T__ size_t;

typedef ssize_t intptr_t;
typedef size_t uintptr_t;

#define NULL ((void*)0)

enum {
  false	= 0,
  true	= 1
};

#endif

#endif

#define NQ_FALSE false
#define NQ_TRUE true

#define NQNotSize ((size_t)-1)
#define NQNonUChar (-1)

typedef bool NQBool;
typedef int32_t NQInt32;
typedef uint32_t NQUint32;

#ifdef NQ_COMPILER_MSVC
typedef __int64 NQInt64;
typedef unsigned __int64 NQUint64;
#else
typedef int64_t NQInt64;
typedef uint64_t NQUint64;
#endif

typedef intptr_t NQIntPtr;
typedef uintptr_t NQUintPtr;

typedef size_t NQSize;
typedef int32_t NQUChar;

#ifdef NQ_OS_WINDOWS
typedef wchar_t NQWChar;
#else
typedef uint16_t NQWChar;
#endif

/*
  NQChar8
  NQChar16
  NQChar32
*/

typedef size_t NQIndex;
typedef int NQOptionFlags;

#ifdef __cplusplus
namespace NQ {
constexpr size_t notFound = static_cast<size_t>(-1);
} // namespace NQ
#define NQNotFound ::NQ::notFound
#else
#define NQNotFound ((size_t)-1)
#endif

#endif /* _LIBNETQ_TYPES_H */
