/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_CPU_ARC_H
#define _LIBNETQ_CPU_ARC_H

#define NQ_FOR_EACH_CPU_ARC(macro) \
  macro(__arc__)

#if  defined (__arc__)

#define NQ_CPU_ARC 1
#define NQ_CPU_NAME "ARC"
#define NQ_CPU_32BIT 1

#if defined(__BIG_ENDIAN__) || defined(__big_endian__)
#define NQ_CPU_BIG_ENDIAN 1
#define NQ_CPU_ARC_ABI_ENDIAN "eb"
#else
#define NQ_CPU_LITTLE_ENDIAN 1
#define NQ_CPU_ARC_ABI_ENDIAN ""
#endif

/* ARCv2 (HS, EM) and ARCompact (ARC600/601/700) are not binary-compatible. The ARCv2 FPU works on core
   registers, so hard float does not change the ABI */
#if defined(__ARCHS__) || defined(__ARCEM__)
# define NQ_CPU_ABI "arcv2" NQ_CPU_ARC_ABI_ENDIAN
#else
# define NQ_CPU_ABI "arcompact" NQ_CPU_ARC_ABI_ENDIAN
#endif

#endif

#endif /* _LIBNETQ_CPU_ARC_H */
