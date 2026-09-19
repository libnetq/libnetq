/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_LIMITS_H
#define _LIBNETQ_LIMITS_H

#include <libnetq/OS.h>
#include <libnetq/ConstExpr.h>

#ifdef NQ_OS_KERNEL
#include <linux/limits.h>
#else
#include <limits.h>
#endif

/* CHAR */
#if defined(CHAR_BIT)
#define NQ_CHAR_BIT CHAR_BIT
#else
#define NQ_CHAR_BIT (8)
#endif

/* INT */
#define NQ_INT_MIN INT_MIN
#define NQ_INT_MAX INT_MAX
#if defined(INT_BIT)
#define NQ_INT_BIT INT_BIT
#else
#define NQ_INT_BIT (32)
#endif

/* UINT */
#define NQ_UINT_MIN (0)
#define NQ_UINT_MAX UINT_MAX

/* LONG */
#define NQ_LONG_MIN LONG_MIN
#define NQ_LONG_MAX LONG_MAX

/* ULONG */
#define NQ_ULONG_MIN (0)
#define NQ_ULONG_MAX ULONG_MAX

/* LLONG */
#define NQ_LLONG_MIN LLONG_MIN
#define NQ_LLONG_MAX LLONG_MAX

/* ULLONG */
#define NQ_ULLONG_MIN (0)
#define NQ_ULLONG_MAX ULLONG_MAX

/* INT8 */
#if defined(_I8_MIN)
#define NQ_INT8_MIN _I8_MIN
#elif defined(SCHAR_MIN)
#define NQ_INT8_MIN SCHAR_MIN
#else
#define NQ_INT8_MIN (-127-1)
#endif

#if defined(_I8_MAX)
#define NQ_INT8_MAX _I8_MAX
#elif defined(SCHAR_MAX)
#define NQ_INT8_MAX SCHAR_MAX
#else
#define NQ_INT8_MAX (127)
#endif

/* UINT8 */
#define NQ_UINT8_MIN (0)

#if defined(_UI8_MAX)
#define NQ_UINT8_MAX _UI8_MAX
#elif defined(UCHAR_MAX)
#define NQ_UINT8_MAX UCHAR_MAX
#else
#define NQ_UINT8_MAX (255)
#endif

/* INT16 */
#if defined(_I16_MIN)
#define NQ_INT16_MIN _I16_MIN
#elif defined(SHRT_MIN)
#define NQ_INT16_MIN SHRT_MIN
#else
#define NQ_INT16_MIN (-32767-1)
#endif

#if defined(_I16_MAX)
#define NQ_INT16_MAX _I16_MAX
#elif defined(SHRT_MAX)
#define NQ_INT16_MAX SHRT_MAX
#else
#define NQ_INT16_MAX (32767)
#endif

/* UINT16 */
#define NQ_UINT16_MIN (0)

#if defined(_UI16_MAX)
#define NQ_UINT16_MAX _UI16_MAX
#elif defined(USHRT_MAX)
#define NQ_UINT16_MAX USHRT_MAX
#else
#define NQ_UINT16_MAX (65535)
#endif

/* INT32 */
#if defined(_I32_MIN)
#define NQ_INT32_MIN _I32_MIN
#elif defined(INT_MIN)
#define NQ_INT32_MIN INT_MIN
#else
#define NQ_INT32_MIN (-2147483647-1)
#endif

#if defined(_I32_MAX)
#define NQ_INT32_MAX _I32_MAX
#elif defined(INT_MAX)
#define NQ_INT32_MAX INT_MAX
#else
#define NQ_INT32_MAX (2147483647)
#endif

/* UINT32 */
#define NQ_UINT32_MIN (0)

#if defined(_UI32_MAX)
#define NQ_UINT32_MAX _UI32_MAX
#elif defined(UINT_MAX)
#define NQ_UINT32_MAX UINT_MAX
#else
#define NQ_UINT32_MAX (4294967295)
#endif

/* INT64 */
#if defined(_I64_MIN)
# define NQ_INT64_MIN _I64_MIN
#else
# define NQ_INT64_MIN (-NQ_INT64_C(9223372036854775807)-1)
#endif

#if defined(_I64_MAX)
# define NQ_INT64_MAX _I64_MAX
#else
# define NQ_INT64_MAX (NQ_INT64_C(9223372036854775807))
#endif

/* UINT64 */
#define NQ_UINT64_MIN (0)

#if defined(_UI64_MAX)
# define NQ_UINT64_MAX _UI64_MAX
#else
# define NQ_UINT64_MAX (NQ_UINT64_C(18446744073709551615))
#endif

/* UINTPTR_MAX */
#if defined(UINTPTR_MAX)
#define NQ_UINTPTR_MAX UINTPTR_MAX
#elif defined(__LP64__) || defined(_WIN64)
#define NQ_UINTPTR_MAX NQ_UINT64_MAX
#else
#define NQ_UINTPTR_MAX NQ_UINT32_MAX
#endif

#endif /* _LIBNETQ_LIMITS_H */
