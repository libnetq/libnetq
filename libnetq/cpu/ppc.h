/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_CPU_PPC_H
#define _LIBNETQ_CPU_PPC_H

#define NQ_FOR_EACH_CPU_PPC(macro) \
  macro(__powerpc)       \
  macro(__powerpc__)     \
  macro(__powerpc64__)   \
  macro(__POWERPC__)     \
  macro(__ppc)           \
  macro(__ppc__)         \
  macro(__ppc64__)       \
  macro(__PPC__)         \
  macro(__PPC64__)       \
  macro(_ARCH_PPC)       \
  macro(_ARCH_PPC64)     \
  macro(_ARCH_440)       \
  macro(_ARCH_450)       \
  macro(_ARCH_601)       \
  macro(__ppc601__)      \
  macro(_ARCH_603)       \
  macro(__ppc603__)      \
  macro(_ARCH_604)       \
  macro(__ppc604__)      \
  macro(_M_PPC)          \
  macro(__PPCGECKO__)    \
  macro(__PPCBROADWAY__) \
  macro(_XENON)

#if  defined (__ppc)           \
  || defined (__powerpc)       \
  || defined (__powerpc__)     \
  || defined (__powerpc64__)   \
  || defined (__POWERPC__)     \
  || defined (__ppc__)         \
  || defined (__ppc64__)       \
  || defined (__PPC__)         \
  || defined (__PPC64__)       \
  || defined (_ARCH_PPC)       \
  || defined (_ARCH_PPC64)     \
  || defined (_ARCH_440)       \
  || defined (_ARCH_450)       \
  || defined (_ARCH_601)       \
  || defined (__ppc601__)      \
  || defined (_ARCH_603)       \
  || defined (__ppc603__)      \
  || defined (_ARCH_604)       \
  || defined (__ppc604__)      \
  || defined (_M_PPC)          \
  || defined (__PPCGECKO__)    \
  || defined (__PPCBROADWAY__) \
  || defined (_XENON)

#define NQ_CPU_PPC 1
#define NQ_CPU_NAME "PowerPC"

/* Windows NT on PowerPC is little-endian, Xbox 360 (_M_PPCBE) is big-endian */
#if  defined(__LITTLE_ENDIAN__) \
  || (defined(_M_PPC) && !defined(_M_PPCBE) && !defined(_XENON))
#define NQ_CPU_LITTLE_ENDIAN 1
#else
#define NQ_CPU_BIG_ENDIAN 1
#endif

/* _ARCH_PPC64 means 64-bit instructions, not 64-bit pointers: -m32 -mcpu=970/e5500 defines it with 32-bit pointers */
#if  defined(__powerpc64__) \
  || defined(__ppc64__)     \
  || defined(__PPC64__)     \
  || defined(__64BIT__)
/* IEEE binary128 long double (-mabi=ieeelongdouble) is not interchangeable with the IBM double-double default */
# if defined(__LONG_DOUBLE_IEEE128__)
#  define NQ_CPU_PPC_ABI_LDBL " (ieee128)"
# else
#  define NQ_CPU_PPC_ABI_LDBL ""
# endif
# if defined(NQ_CPU_LITTLE_ENDIAN)
#  define NQ_CPU_ABI "ppc64le" NQ_CPU_PPC_ABI_LDBL
# elif defined(_CALL_ELF) && _CALL_ELF == 2
#  define NQ_CPU_ABI "ppc64-elfv2" NQ_CPU_PPC_ABI_LDBL
# else
#  define NQ_CPU_ABI "ppc64" NQ_CPU_PPC_ABI_LDBL
# endif
#define NQ_CPU_64BIT 1
#else
# if defined(NQ_CPU_LITTLE_ENDIAN)
#  define NQ_CPU_ABI "ppcle"
# elif defined(_SOFT_FLOAT) || defined(__NO_FPRS__)
#  define NQ_CPU_ABI "ppc (soft-float)"
# else
#  define NQ_CPU_ABI "ppc"
# endif
#define NQ_CPU_32BIT 1
#endif

#endif

#endif /* _LIBNETQ_CPU_PPC_H */
