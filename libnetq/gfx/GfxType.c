/*
 * MIT License
 *
 * Copyright (c) 2024-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/gfx/GfxType.h"

#include <libnetq/Assert.h>

const char* NQLineCapToString(NQLineCap lineCap)
{
  switch (lineCap) {
  case NQ_LINECAP_BUTT:
    return "butt";
  case NQ_LINECAP_ROUND:
    return "round";
  case NQ_LINECAP_SQUARE:
    return "square";
  }

  NQ_ASSERT_NOT_REACHED();
  return NULL;
}

const char* NQLineJoinToString(NQLineJoin lineJoin)
{
  switch (lineJoin) {
  case NQ_LINEJOIN_MITER:
    return "miter";
  case NQ_LINEJOIN_ROUND:
    return "round";
  case NQ_LINEJOIN_BEVEL:
    return "bevel";
  }

  NQ_ASSERT_NOT_REACHED();
  return NULL;
}
