/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_BITOPS_H
#define _LIBNETQ_BITOPS_H

#include <libnetq/Basic.h>
#include <libnetq/CPU.h>
#include <libnetq/Compiler.h>

#ifdef NQ_OS_KERNEL
#include <linux/bitops.h>
#endif

#ifdef NQ_OS_WINDOWS
#include <intrin.h>
#endif

#if NQ_HAS_BUILTIN(__builtin_clz)
static NQ_ALWAYS_INLINE unsigned NQGetClz32(uint32_t x)
{
  return x ? __builtin_clz(x) : sizeof(x) * 8;
}
#elif defined(NQ_COMPILER_MSVC) && defined(NQ_CPU_64BIT)
static NQ_ALWAYS_INLINE unsigned NQGetClz32(uint32_t x)
{
  unsigned long ret = 0;
  if (_BitScanForward(&ret, x))
    return ret;
  return 32;
}
#else
static unsigned NQGetClz32(uint32_t number)
{
  unsigned zeroCount = 0;
  for (int i = 31; i >= 0; i--) {
    if (!(number >> i))
      zeroCount++;
    else
      break;
  }
  return zeroCount;
}
#endif

#if NQ_HAS_BUILTIN(__builtin_clzl) && NQ_HAS_BUILTIN(__builtin_clzll)
# if defined(__LP64__) || (defined(__WORDSIZE) && __WORDSIZE == 64)
static NQ_ALWAYS_INLINE unsigned NQGetClz64(uint64_t x)
{
  return x ? __builtin_clzl(x) : sizeof(x) * 8;
}
# else
static NQ_ALWAYS_INLINE unsigned NQGetClz64(uint64_t x)
{
  return x ? __builtin_clzll(x) : sizeof(x) * 8;
}
# endif
#elif defined(NQ_COMPILER_MSVC) && defined(NQ_CPU_64BIT) && !defined(NQ_CPU_ARM)
static NQ_ALWAYS_INLINE unsigned NQGetClz64(uint64_t x)
{
  unsigned long ret = 0;
  if (_BitScanReverse64(&ret, x))
    return 63 - ret;
  return 64;
}
#else
static unsigned NQGetClz64(uint64_t x)
{
  unsigned zeroCount = 0;
  for (int i = 63; i >= 0; i--) {
    if (!(x >> i))
      zeroCount++;
    else
      break;
  }
  return zeroCount;
}
#endif

#ifdef NQ_OS_KERNEL
static NQ_ALWAYS_INLINE unsigned NQGetFls32(uint32_t x)
{
  return fls(x);
}

static NQ_ALWAYS_INLINE unsigned NQGetFls64(uint64_t x)
{
  return fls64(x);
}
#else
static NQ_ALWAYS_INLINE unsigned NQGetFls32(uint32_t x)
{
  return sizeof(x) * 8 - NQGetClz32(x);
}

static NQ_ALWAYS_INLINE unsigned NQGetFls64(uint64_t x)
{
  return sizeof(x) * 8 - NQGetClz64(x);
}
#endif

#endif /* _LIBNETQ_BITOPS_H */
