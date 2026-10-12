/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_CPU_MICROBLAZE_H
#define _LIBNETQ_CPU_MICROBLAZE_H

#define NQ_FOR_EACH_CPU_MICROBLAZE(macro) \
  macro(__microblaze__)   \
  macro(__MICROBLAZE__)   \
  macro(__MICROBLAZEEB__) \
  macro(__MICROBLAZEEL__)

#if  defined (__microblaze__)   \
  || defined (__MICROBLAZE__)   \
  || defined (__MICROBLAZEEB__) \
  || defined (__MICROBLAZEEL__)

#define NQ_CPU_MICROBLAZE 1
#define NQ_CPU_NAME "MicroBlaze"
#define NQ_CPU_32BIT 1

/* The FPU (-mhard-float) works on general registers, so it does not change the ABI */
#if defined(__MICROBLAZEEL__) || defined(__LITTLE_ENDIAN__)
#define NQ_CPU_ABI "microblazeel"
#define NQ_CPU_LITTLE_ENDIAN 1
#else
#define NQ_CPU_ABI "microblaze"
#define NQ_CPU_BIG_ENDIAN 1
#endif

#endif

#endif /* _LIBNETQ_CPU_MICROBLAZE_H */
