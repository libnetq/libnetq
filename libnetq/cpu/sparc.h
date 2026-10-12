/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_CPU_SPARC_H
#define _LIBNETQ_CPU_SPARC_H

#define NQ_FOR_EACH_CPU_SPARC(macro) \
  macro(__sparc__)        \
  macro(__sparc)          \
  macro(__sparc_v8__)     \
  macro(__sparcv8)        \
  macro(__sparcv8plus)    \
  macro(__sparc_v8plus__) \
  macro(__sparc_v9__)     \
  macro(__sparcv9)
#if  defined (__sparc__)        \
  || defined (__sparc)          \
  || defined (__sparc_v8__)     \
  || defined (__sparcv8)        \
  || defined (__sparcv8plus)    \
  || defined (__sparc_v8plus__) \
  || defined (__sparc_v9__)     \
  || defined (__sparcv9)

#define NQ_CPU_SPARC 1
#define NQ_CPU_NAME "SPARC"
#define NQ_CPU_BIG_ENDIAN 1

/* __sparc_v9__ only means the V9 instruction set (32-bit V8+ code defines it too); the 64-bit ABI is __arch64__
   (gcc) or __sparcv9 (Sun cc) */
#if defined(__arch64__) || defined(__sparcv9) || defined(__LP64__) || defined(_LP64)
#define NQ_CPU_ABI "sparc64"
#define NQ_CPU_64BIT 1
#else
#define NQ_CPU_ABI "sparc"
#define NQ_CPU_32BIT 1
#endif

#endif

#endif /* _LIBNETQ_CPU_SPARC_H */
