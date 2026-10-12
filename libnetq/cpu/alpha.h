/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_CPU_ALPHA_H
#define _LIBNETQ_CPU_ALPHA_H

#define NQ_FOR_EACH_CPU_ALPHA(macro) \
  macro(__alpha__)     \
  macro(__alpha)       \
  macro(_M_ALPHA)      \
  macro(__alpha_ev4__) \
  macro(__alpha_ev5__) \
  macro(__alpha_ev6__)

#if  defined (__alpha__)     \
  || defined (__alpha)       \
  || defined (_M_ALPHA)      \
  || defined (__alpha_ev4__) \
  || defined (__alpha_ev5__) \
  || defined (__alpha_ev6__)

#define NQ_CPU_ALPHA 1
#define NQ_CPU_NAME "DEC Alpha"
#define NQ_CPU_LITTLE_ENDIAN 1

/* Windows NT on Alpha uses 32-bit pointers; only AXP64 (_WIN64) and Unix-like systems are LP64 */
#if defined(_M_ALPHA) && !defined(_WIN64)
#define NQ_CPU_32BIT 1
#else
#define NQ_CPU_64BIT 1
#endif

/* The ABI is the same on every core, the suffix names the baseline the code was built for:
   ev67 adds CIX (and FIX, MAX), ev6 adds FIX, pca56 adds MAX, ev56 adds BWX byte/word loads and stores */
#if defined(__alpha_ev6__) && defined(__alpha_cix__)
# define NQ_CPU_ABI "alpha-ev67"
#elif defined(__alpha_ev6__)
# define NQ_CPU_ABI "alpha-ev6"
#elif defined(__alpha_ev5__) && defined(__alpha_max__)
# define NQ_CPU_ABI "alpha-pca56"
#elif defined(__alpha_ev5__) && defined(__alpha_bwx__)
# define NQ_CPU_ABI "alpha-ev56"
#elif defined(__alpha_ev5__)
# define NQ_CPU_ABI "alpha-ev5"
#else
# define NQ_CPU_ABI "alpha"
#endif

#endif

#endif /* _LIBNETQ_CPU_ALPHA_H */
