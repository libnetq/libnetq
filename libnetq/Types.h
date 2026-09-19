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
#include <libnetq/Compiler.h>
#include <libnetq/ConstExpr.h>

#ifdef NQ_OS_KERNEL
#include <linux/types.h>
#elif defined(NQ_OS_WINDOWS) || defined(NQ_OS_UNIX)
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
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
