/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_MINMAX_H
#define _LIBNETQ_MINMAX_H

#define NQGetMin(a, b) ((a) > (b) ? (b) : (a))
#define NQGetMax(a, b) ((a) < (b) ? (b) : (a))
#define NQGetClamp(val, min, max) ((val) > (min) ? ((val) < (max) ? (val) : (max)) : (min))
#define NQGetDiff(a, b) (NQGetMax(a, b) - NQGetMin(a, b))

#endif /* _LIBNETQ_MINMAX_H */
