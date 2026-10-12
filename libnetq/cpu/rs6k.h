/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_CPU_RS6K_H
#define _LIBNETQ_CPU_RS6K_H

#define NQ_FOR_EACH_CPU_RS6K(macro) \
  macro(__THW_RS6000) \
  macro(_IBMR2)       \
  macro(_POWER)       \
  macro(_ARCH_PWR)    \
  macro(_ARCH_PWR2)   \
  macro(_ARCH_PWR3)   \
  macro(_ARCH_PWR4)

/* PowerPC compilers define _ARCH_PWR4+ for POWER4 and newer cores (e.g. ppc64le POWER8), and AIX xlc defines
   _IBMR2/_POWER on PowerPC too; those are detected by ppc.h (included first) */
#if (defined (__THW_RS6000) \
  || defined (_IBMR2)       \
  || defined (_POWER)       \
  || defined (_ARCH_PWR)    \
  || defined (_ARCH_PWR2)   \
  || defined (_ARCH_PWR3)   \
  || defined (_ARCH_PWR4))  \
  && !defined (NQ_CPU_PPC)

#define NQ_CPU_RS6000 1
#define NQ_CPU_NAME "RS/6000"
#define NQ_CPU_BIG_ENDIAN 1

#endif

#endif /* _LIBNETQ_CPU_RS6K_H */
