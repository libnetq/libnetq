/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_GFX_GFXTYPES_H
#define _LIBNETQ_GFX_GFXTYPES_H

#include <libnetq/Basic.h>

typedef uint8_t NQLineCap;
enum {
  kNQLineCapButt,
  kNQLineCapRound,
  kNQLineCapSquare,
};

typedef uint8_t NQLineJoin;
enum {
  kNQLineJoinMiter,
  kNQLineJoinRound,
  kNQLineJoinBevel,
};

#endif /* _LIBNETQ_GFX_GFXTYPES_H */
