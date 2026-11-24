/*
 * MIT License
 *
 * Copyright (c) 2021-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_ALGO_GETSINEWAVE_H
#define _LIBNETQ_ALGO_GETSINEWAVE_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQSineWave {
  double a; /* 1.0 */
  double k; /* 0.0 */
  double h; /* 0.0 */
  double b; /* 1.0 */
} NQSineWave;

NQ_EXPORT double NQSineWave_calc(const NQSineWave*, double x);
NQ_EXPORT double NQGetSineWave(double a, double k, double h, double b, double x);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_ALGO_GETSINEWAVE_H */
