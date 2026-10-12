/*
 * MIT License
 *
 * Copyright (c) 2021-2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/GetSineWave.h"

#include <libnetq/Math.h>

#if defined(NQ_OS_WINDOWS) || defined(NQ_OS_UNIX)
#define NQSin(x) sin(x)
#else
// FIXME
#define NQSin(x) (x)
#endif

double NQSineWave_calc(const NQSineWave* thiz, double x)
{
  return NQGetSineWave(thiz->a, thiz->k, thiz->h, thiz->b, x);
}

double NQGetSineWave(double a, double k, double h, double b, double x)
{
  if (b == 0.0)
    return 0.0;

  return a * NQSin((x - h) / b) + k;
}
