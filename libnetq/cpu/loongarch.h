/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_CPU_LOONGARCH_H
#define _LIBNETQ_CPU_LOONGARCH_H

#define NQ_FOR_EACH_CPU_LOONGARCH(macro) \
  macro(__loongarch__)     \
  macro(__loongarch64)     \
  macro(__loongarch_grlen)

#if  defined (__loongarch__)  \
  || defined (__loongarch64)  \
  || defined (__loongarch_grlen)

#define NQ_CPU_LOONGARCH 1
#define NQ_CPU_NAME "LoongArch"
#define NQ_CPU_LITTLE_ENDIAN 1

#if defined(__loongarch_grlen)
# if __loongarch_grlen == 64
#  define NQ_CPU_64BIT 1
# elif __loongarch_grlen == 32
#  define NQ_CPU_32BIT 1
# else
#  error Not supported LoongArch bit size
# endif
#elif defined(__loongarch64)
# define NQ_CPU_64BIT 1
#endif

/* psABI name: integer ABI (lp64/ilp32) + float ABI suffix (d: 64-bit FPU, f: 32-bit FPU, s: soft float) */
#if defined(__loongarch_double_float)
# define NQ_CPU_LOONGARCH_ABI_FLOAT "d"
#elif defined(__loongarch_single_float)
# define NQ_CPU_LOONGARCH_ABI_FLOAT "f"
#else
# define NQ_CPU_LOONGARCH_ABI_FLOAT "s"
#endif
#if defined(NQ_CPU_64BIT)
# define NQ_CPU_ABI "loongarch64-lp64" NQ_CPU_LOONGARCH_ABI_FLOAT
#elif defined(NQ_CPU_32BIT)
# define NQ_CPU_ABI "loongarch32-ilp32" NQ_CPU_LOONGARCH_ABI_FLOAT
#else
# define NQ_CPU_ABI "loongarch"
#endif

#endif

#endif /* _LIBNETQ_CPU_LOONGARCH_H */
