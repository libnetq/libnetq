/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_CPU_ARM64_H
#define _LIBNETQ_CPU_ARM64_H

#define NQ_FOR_EACH_CPU_ARM64(macro) \
  macro(__aarch64__) \
  macro(__arm64__)

#if  defined (__aarch64__) \
  || defined (__arm64__)

#define NQ_CPU_ARM64 1
#define NQ_CPU_NAME "ARM64"

#if defined(__AARCH64EB__) || defined(__ARM_BIG_ENDIAN)
#define NQ_CPU_BIG_ENDIAN 1
#else
#define NQ_CPU_LITTLE_ENDIAN 1
#endif

/* ILP32 ABI runs AArch64 code with 32-bit pointers */
#if defined(__ILP32__) || defined(_ILP32)
#define NQ_CPU_ABI "arm64-ilp32"
#define NQ_CPU_32BIT 1
#else
#define NQ_CPU_ABI "arm64-v8a"
#define NQ_CPU_64BIT 1
#endif

#endif

#endif /* _LIBNETQ_CPU_ARM64_H */
