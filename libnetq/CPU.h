/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_CPU_H
#define _LIBNETQ_CPU_H

#include <libnetq/Basic.h>

#include <libnetq/cpu/alpha.h>
#include <libnetq/cpu/amd64.h>
#include <libnetq/cpu/arm.h>
#include <libnetq/cpu/arm64.h>
#include <libnetq/cpu/blackfin.h>
#include <libnetq/cpu/convex.h>
#include <libnetq/cpu/epiphany.h>
#include <libnetq/cpu/hppa.h>
#include <libnetq/cpu/ia64.h>
#include <libnetq/cpu/m68k.h>
#include <libnetq/cpu/mips.h>
#include <libnetq/cpu/ppc.h>
#include <libnetq/cpu/pyramid.h>
#include <libnetq/cpu/riscv.h>
#include <libnetq/cpu/rs6k.h>
#include <libnetq/cpu/s370.h>
#include <libnetq/cpu/s390.h>
#include <libnetq/cpu/sh.h>
#include <libnetq/cpu/sparc.h>
#include <libnetq/cpu/tms320.h>
#include <libnetq/cpu/tms470.h>
#include <libnetq/cpu/wasm.h>
#include <libnetq/cpu/x86.h>
#include <libnetq/cpu/z.h>

#if defined(__BYTE_ORDER__)
# if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#  if defined(NQ_CPU_LITTLE_ENDIAN)
#   error "Conflicting CPU endianness: NQ_CPU_LITTLE_ENDIAN defined but __BYTE_ORDER__ is big-endian"
#  elif !defined(NQ_CPU_BIG_ENDIAN)
#   define NQ_CPU_BIG_ENDIAN 1
#  endif
# elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  if defined(NQ_CPU_BIG_ENDIAN)
#   error "Conflicting CPU endianness: NQ_CPU_BIG_ENDIAN defined but __BYTE_ORDER__ is little-endian"
#  elif !defined(NQ_CPU_LITTLE_ENDIAN)
#   define NQ_CPU_LITTLE_ENDIAN 1
#  endif
# else
#  error "Unsupported byte order: only big-endian and little-endian are supported"
# endif
#endif

#ifdef __SIZEOF_POINTER__
# if __SIZEOF_POINTER__ == 8
#  if defined(NQ_CPU_32BIT)
#   error "Conflicting CPU architecture: NQ_CPU_32BIT defined but pointer size is 64-bit"
#  elif !defined(NQ_CPU_64BIT)
#   define NQ_CPU_64BIT 1
#  endif
# elif __SIZEOF_POINTER__ == 4
#  if defined(NQ_CPU_64BIT)
#   error "Conflicting CPU architecture: NQ_CPU_64BIT defined but pointer size is 32-bit"
#  elif !defined(NQ_CPU_32BIT)
#   define NQ_CPU_32BIT 1
#  endif
# else
#   error "Unsupported pointer size: only 32-bit and 64-bit architectures are supported"
# endif
#endif

# ifdef __LP64__
#  if defined(NQ_CPU_32BIT)
#   error "Conflicting CPU architecture: NQ_CPU_32BIT defined but pointer size is 64-bit"
#  elif !defined(NQ_CPU_64BIT)
#   define NQ_CPU_64BIT 1
#  endif
# endif

#ifdef NQ_CPU_STRICT_MODE
# ifndef NQ_CPU_NAME
#  error "NQ_CPU_STRICT_MODE: NQ_CPU_NAME is not defined"
# endif
# if !defined(NQ_CPU_LITTLE_ENDIAN) && !defined(NQ_CPU_BIG_ENDIAN)
#  error "NQ_CPU_STRICT_MODE: CPU endianness could not be detected"
# endif
# if !defined(NQ_CPU_64BIT) && !defined(NQ_CPU_32BIT)
#  error "NQ_CPU_STRICT_MODE: CPU word size could not be detected"
# endif
#endif

#ifndef NQ_CPU_NAME
# define NQ_CPU_NAME "Unknown"
#endif

#ifndef NQ_CPU_ABI
# define NQ_CPU_ABI "unknown"
#endif

#if !defined(NQ_CPU_LITTLE_ENDIAN) && !defined(NQ_CPU_BIG_ENDIAN)
# define NQ_CPU_LITTLE_ENDIAN 1
#endif

#if !defined(NQ_CPU_64BIT) && !defined(NQ_CPU_32BIT)
# define NQ_CPU_32BIT 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

NQ_EXPORT unsigned NQGetNumberOfProcessors(void);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_CPU_H */
