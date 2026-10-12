/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_CPU_ARM_H
#define _LIBNETQ_CPU_ARM_H

#define NQ_FOR_EACH_CPU_ARM(macro) \
  macro(__arm)               \
  macro(__arm__)             \
  macro(__thumb__)           \
  macro(__ARM_ARCH_2__)      \
  macro(__ARM_ARCH_3__)      \
  macro(__ARM_ARCH_3M__)     \
  macro(__ARM_ARCH_4T__)     \
  macro(__TARGET_ARM_4T)     \
  macro(__ARM_ARCH_5__)      \
  macro(__ARM_ARCH_5E__)     \
  macro(__ARM_ARCH_5T__)     \
  macro(__ARM_ARCH_5TE__)    \
  macro(__ARM_ARCH_5TEJ__)   \
  macro(__ARM_ARCH_6__)      \
  macro(__ARM_ARCH_6J__)     \
  macro(__ARM_ARCH_6K__)     \
  macro(__ARM_ARCH_6Z__)     \
  macro(__ARM_ARCH_6ZK__)    \
  macro(__ARM_ARCH_6T2__)    \
  macro(__ARM_ARCH_7__)      \
  macro(__ARM_ARCH_7A__)     \
  macro(__ARM_ARCH_7R__)     \
  macro(__ARM_ARCH_7M__)     \
  macro(__ARM_ARCH_7S__)     \
  macro(__TARGET_ARCH_ARM)   \
  macro(__TARGET_ARCH_THUMB) \
  macro(_ARM)                \
  macro(_M_ARM)              \
  macro(_M_ARMT)

#if  defined (__arm)             \
  || defined (__arm__)           \
  || defined (__thumb__)         \
  || defined (__ARM_ARCH_2__)    \
  || defined (__ARM_ARCH_3__)    \
  || defined (__ARM_ARCH_3M__)   \
  || defined (__ARM_ARCH_4T__)   \
  || defined (__TARGET_ARM_4T)   \
  || defined (__ARM_ARCH_5__)    \
  || defined (__ARM_ARCH_5E__)   \
  || defined (__ARM_ARCH_5T__)   \
  || defined (__ARM_ARCH_5TE__)  \
  || defined (__ARM_ARCH_5TEJ__) \
  || defined (__ARM_ARCH_6__)    \
  || defined (__ARM_ARCH_6J__)   \
  || defined (__ARM_ARCH_6K__)   \
  || defined (__ARM_ARCH_6Z__)   \
  || defined (__ARM_ARCH_6ZK__)  \
  || defined (__ARM_ARCH_6T2__)  \
  || defined (__ARM_ARCH_7__)    \
  || defined (__ARM_ARCH_7A__)   \
  || defined (__ARM_ARCH_7R__)   \
  || defined (__ARM_ARCH_7M__)   \
  || defined (__ARM_ARCH_7S__)   \
  || defined (__TARGET_ARCH_ARM)   \
  || defined (__TARGET_ARCH_THUMB) \
  || defined (_ARM)              \
  || defined (_M_ARM)            \
  || defined (_M_ARMT)

#define NQ_CPU_ARM 1
#define NQ_CPU_NAME "ARM"
#define NQ_CPU_32BIT 1

/* __ARMEB__ covers both BE32 (ARMv5 and older, e.g. XScale IXP4xx) and BE8 (ARMv6+); Windows on ARM is little-endian */
#if defined(__ARMEB__) || defined(__ARM_BIG_ENDIAN)
#define NQ_CPU_BIG_ENDIAN 1
#else
#define NQ_CPU_LITTLE_ENDIAN 1
#endif

/* ARMv8-A and later in AArch32 state: every 8.x revision keeps base 8A, so gcc defines __ARM_ARCH_8A__ for all */
#if  defined(__ARM_ARCH_8A__) \
  || (defined(__ARM_ARCH) && __ARM_ARCH >= 8 && defined(__ARM_ARCH_PROFILE) && __ARM_ARCH_PROFILE == 'A')
# define NQ_CPU_ARM_ABI_ARCH "armeabi-v8a"
#elif defined(__ARM_ARCH_7A__)
# define NQ_CPU_ARM_ABI_ARCH "armeabi-v7a"
#endif

#if defined(__ARM_ARCH_6__)
# define NQ_CPU_ABI "armeabi-v6"
#elif defined(NQ_CPU_ARM_ABI_ARCH)
# if defined(__ARM_NEON__)
#  define NQ_CPU_ARM_ABI_NEON "/NEON"
# else
#  define NQ_CPU_ARM_ABI_NEON ""
# endif
# if defined(__ARM_PCS_VFP)
#  define NQ_CPU_ARM_ABI_FLOAT " (hard-float)"
# else
#  define NQ_CPU_ARM_ABI_FLOAT ""
# endif
# define NQ_CPU_ABI NQ_CPU_ARM_ABI_ARCH NQ_CPU_ARM_ABI_NEON NQ_CPU_ARM_ABI_FLOAT
#else
# define NQ_CPU_ABI "armeabi"
#endif

#endif

#endif /* _LIBNETQ_CPU_ARM_H */
