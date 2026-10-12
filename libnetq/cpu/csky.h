/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_CPU_CSKY_H
#define _LIBNETQ_CPU_CSKY_H

#define NQ_FOR_EACH_CPU_CSKY(macro) \
  macro(__csky__)      \
  macro(__CSKY__)      \
  macro(__ckcore__)    \
  macro(__CKCORE__)    \
  macro(__cskyabi__)   \
  macro(__CSKYABI__)

#if  defined (__csky__)    \
  || defined (__CSKY__)    \
  || defined (__ckcore__)  \
  || defined (__CKCORE__)  \
  || defined (__cskyabi__) \
  || defined (__CSKYABI__)

#define NQ_CPU_CSKY 1
#define NQ_CPU_NAME "C-SKY"
#define NQ_CPU_32BIT 1

#if defined(__CSKYBE__) || defined(__ckcoreBE__)
#define NQ_CPU_BIG_ENDIAN 1
#define NQ_CPU_CSKY_ABI_ENDIAN "-be"
#else
#define NQ_CPU_LITTLE_ENDIAN 1
#define NQ_CPU_CSKY_ABI_ENDIAN ""
#endif

/* Named after the glibc dynamic linker: -mfloat-abi=softfp uses the FPU but keeps the soft-float ABI */
#if defined(__CSKY_HARD_FLOAT_ABI__)
# define NQ_CPU_CSKY_ABI_FLOAT "-hf"
#else
# define NQ_CPU_CSKY_ABI_FLOAT ""
#endif
#if defined(__CSKYABIV2__) || (defined(__CSKYABI__) && __CSKYABI__ == 2)
# define NQ_CPU_ABI "cskyv2" NQ_CPU_CSKY_ABI_FLOAT NQ_CPU_CSKY_ABI_ENDIAN
#else
# define NQ_CPU_ABI "cskyv1" NQ_CPU_CSKY_ABI_ENDIAN
#endif

#endif

#endif /* _LIBNETQ_CPU_CSKY_H */
