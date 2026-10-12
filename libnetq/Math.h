/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_MATH_H
#define _LIBNETQ_MATH_H

#include <libnetq/Basic.h>
#include <libnetq/MinMax.h>

#if defined(NQ_OS_KERNEL)
#include <linux/math.h>
#include <linux/math64.h>
#elif defined(NQ_OS_WINDOWS) || defined(NQ_OS_UNIX)
#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define NQ_PI M_PI

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif
#define NQ_PI_2 M_PI_2

#ifndef M_PI_4
#define M_PI_4 0.785398163397448309616
#endif
#define NQ_PI_4 M_PI_4

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif
#define NQ_SQRT2 M_SQRT2

/* Greatest Common Divisor */
NQ_EXPORT int64_t nq_gcd64(int64_t a, int64_t b);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_MATH_H */
