/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_CPU_MIPS_H
#define _LIBNETQ_CPU_MIPS_H

#define NQ_FOR_EACH_CPU_MIPS(macro) \
  macro(__mips__)        \
  macro(mips)            \
  macro(_R3000)          \
  macro(_R4000)          \
  macro(_R5900)          \
  macro(__mips)          \
  macro(__mips64)        \
  macro(__MIPS__)        \
  macro(_MIPS_ISA)       \
  macro(_MIPS_ISA_MIPS1) \
  macro(_MIPS_ISA_MIPS2) \
  macro(_MIPS_ISA_MIPS3) \
  macro(_MIPS_ISA_MIPS4) \
  macro(__MIPS_ISA2__)   \
  macro(__MIPS_ISA3__)   \
  macro(__MIPS_ISA4__)

#if  defined(__mips__)        \
  || defined(mips)            \
  || defined(_R3000)          \
  || defined(_R4000)          \
  || defined(_R5900)          \
  || defined(__mips)          \
  || defined(__mips64)        \
  || defined(__MIPS__)        \
  || defined(_MIPS_ISA)       \
  || defined(_MIPS_ISA_MIPS1) \
  || defined(_MIPS_ISA_MIPS2) \
  || defined(_MIPS_ISA_MIPS3) \
  || defined(_MIPS_ISA_MIPS4) \
  || defined(__MIPS_ISA2__)   \
  || defined(__MIPS_ISA3__)   \
  || defined(__MIPS_ISA4__)

#define NQ_CPU_MIPS 1
#define NQ_CPU_NAME "MIPS"

#if defined(__MIPSEB__)
#define NQ_CPU_BIG_ENDIAN 1
#else
#define NQ_CPU_LITTLE_ENDIAN 1
#endif

/* __mips64 means 64-bit registers, not 64-bit pointers: n32 has both __mips64 and 32-bit pointers */
#if defined(_MIPS_SIM) && defined(_ABI64) && _MIPS_SIM == _ABI64
#define NQ_CPU_ABI "mips64"
#define NQ_CPU_64BIT 1
#elif defined(_MIPS_SIM) && defined(_ABIN32) && _MIPS_SIM == _ABIN32
#define NQ_CPU_ABI "mipsn32"
#define NQ_CPU_32BIT 1
#elif defined(_MIPS_SIM)
#define NQ_CPU_ABI "mips"
#define NQ_CPU_32BIT 1
#elif defined(__LP64__) || defined(__mips64)
#define NQ_CPU_ABI "mips64"
#define NQ_CPU_64BIT 1
#else
#define NQ_CPU_ABI "mips"
#define NQ_CPU_32BIT 1
#endif

#endif

#endif /* _LIBNETQ_CPU_MIPS_H */
