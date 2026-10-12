/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_CPU_ZARCH_H
#define _LIBNETQ_CPU_ZARCH_H

#define NQ_FOR_EACH_CPU_ZARCH(macro) \
  macro(__zarch__) \
  macro(__SYSC_ZARCH__)

/* gcc and clang also define __zarch__ for s390x and for -m31 -mzarch; those are handled by s390.h */
#if (defined (__zarch__) \
  || defined (__SYSC_ZARCH__)) && !defined (__s390__)

#define NQ_CPU_ZARCH 1
#define NQ_CPU_NAME "z/Architecture"
#define NQ_CPU_ABI "zarch"
#define NQ_CPU_BIG_ENDIAN 1
/* z/OS also runs 31-bit (AMODE 31) code */
#if defined(_LP64) || defined(__LP64__)
#define NQ_CPU_64BIT 1
#else
#define NQ_CPU_32BIT 1
#endif

#endif

#endif /* _LIBNETQ_CPU_ZARCH_H */
