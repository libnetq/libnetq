/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_CPU_M68K_H
#define _LIBNETQ_CPU_M68K_H

#define NQ_FOR_EACH_CPU_M68K(macro) \
  macro(__m68k__)    \
  macro(M68000)      \
  macro(__MC68K__)   \
  macro(mc68000)     \
  macro(__mc68000)   \
  macro(__mc68000__) \
  macro(__MC68000__) \
  macro(mc68010)     \
  macro(__mc68010)   \
  macro(__mc68010__) \
  macro(__MC68010__) \
  macro(mc68020)     \
  macro(__mc68020)   \
  macro(__mc68020__) \
  macro(__MC68020__) \
  macro(mc68030)     \
  macro(__mc68030)   \
  macro(__mc68030__) \
  macro(__MC68030__) \
  macro(mc68040)     \
  macro(__mc68040)   \
  macro(__mc68040__) \
  macro(__MC68040__) \
  macro(mc68060)     \
  macro(__mc68060)   \
  macro(__mc68060__) \
  macro(__MC68060__) \
  macro(__mcoldfire__)

#if  defined (__m68k__)    \
  || defined (M68000)      \
  || defined (__MC68K__)   \
  || defined (mc68000)     \
  || defined (__mc68000)   \
  || defined (__mc68000__) \
  || defined (__MC68000__) \
  || defined (mc68010)     \
  || defined (__mc68010)   \
  || defined (__mc68010__) \
  || defined (__MC68010__) \
  || defined (mc68020)     \
  || defined (__mc68020)   \
  || defined (__mc68020__) \
  || defined (__MC68020__) \
  || defined (mc68030)     \
  || defined (__mc68030)   \
  || defined (__mc68030__) \
  || defined (__MC68030__) \
  || defined (mc68040)     \
  || defined (__mc68040)   \
  || defined (__mc68040__) \
  || defined (__MC68040__) \
  || defined (mc68060)     \
  || defined (__mc68060)   \
  || defined (__mc68060__) \
  || defined (__MC68060__) \
  || defined (__mcoldfire__)

#define NQ_CPU_M68K 1
#define NQ_CPU_NAME "Motorola 68k"
/* ColdFire FPU (__mcffpu__) and 68881/68882 (__HAVE_68881__) are not interchangeable */
#if defined(__mcoldfire__)
# if defined(__mcffpu__)
#  define NQ_CPU_ABI "coldfire"
# else
#  define NQ_CPU_ABI "coldfire (soft-float)"
# endif
#elif defined(__HAVE_68881__) || !defined(__GNUC__)
# define NQ_CPU_ABI "m68k"
#else
# define NQ_CPU_ABI "m68k (soft-float)"
#endif
#define NQ_CPU_BIG_ENDIAN 1
#define NQ_CPU_32BIT 1

#endif

#endif /* _LIBNETQ_CPU_M68K_H */
