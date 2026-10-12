/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_CPU_HPPA_H
#define _LIBNETQ_CPU_HPPA_H

#define NQ_FOR_EACH_CPU_HPPA(macro) \
  macro(__hppa__)    \
  macro(__HPPA__)    \
  macro(__hppa)      \
  macro(_PA_RISC1_0) \
  macro(_PA_RISC1_1) \
  macro(__HPPA11__)  \
  macro(__PA7100__)  \
  macro(_PA_RISC2_0) \
  macro(__RISC2_0__) \
  macro(__HPPA20__)  \
  macro(__PA8000__)

#if defined(__hppa__)     \
  || defined(__HPPA__)    \
  || defined(__hppa)      \
  || defined(_PA_RISC1_0) \
  || defined(_PA_RISC1_1) \
  || defined(__HPPA11__)  \
  || defined(__PA7100__)  \
  || defined(_PA_RISC2_0) \
  || defined(__RISC2_0__) \
  || defined(__HPPA20__)  \
  || defined(__PA8000__)

#define NQ_CPU_HPPA 1
#define NQ_CPU_NAME "HP/PA RISC"
#define NQ_CPU_BIG_ENDIAN 1

/* _PA_RISC2_0 names the instruction set, not the pointer size: 32-bit Linux userland built with -march=2.0 has it
   too. 64-bit code (hppa64 kernels, HP-UX +DD64) is LP64 */
#if defined(__LP64__) || defined(_LP64)
# define NQ_CPU_64BIT 1
# define NQ_CPU_ABI "hppa64"
#else
# define NQ_CPU_32BIT 1
# if  defined(_PA_RISC2_0) \
   || defined(__RISC2_0__) \
   || defined(__HPPA20__)  \
   || defined(__PA8000__)
#  define NQ_CPU_ABI "hppa2.0"
# elif defined(_PA_RISC1_1) \
    || defined(__HPPA11__)  \
    || defined(__PA7100__)
#  define NQ_CPU_ABI "hppa1.1"
# else
#  define NQ_CPU_ABI "hppa"
# endif
#endif

#endif

#endif /* _LIBNETQ_CPU_HPPA_H */
