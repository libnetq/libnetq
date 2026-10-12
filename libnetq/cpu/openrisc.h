/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_CPU_OPENRISC_H
#define _LIBNETQ_CPU_OPENRISC_H

#define NQ_FOR_EACH_CPU_OPENRISC(macro) \
  macro(__or1k__) \
  macro(__OR1K__) \
  macro(__or32__) \
  macro(__OR32__)

#if  defined (__or1k__) \
  || defined (__OR1K__) \
  || defined (__or32__) \
  || defined (__OR32__)

#define NQ_CPU_OPENRISC 1
#define NQ_CPU_NAME "OpenRISC"
#define NQ_CPU_BIG_ENDIAN 1
#define NQ_CPU_32BIT 1

/* The FPU (-mhard-float) works on general registers, so it does not change the ABI */
#define NQ_CPU_ABI "or1k"

#endif

#endif /* _LIBNETQ_CPU_OPENRISC_H */
