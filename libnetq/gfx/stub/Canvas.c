/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/gfx/stub/Canvas.h"

#ifdef CANVAS_BACKEND_STUB

#include <libnetq/gfx/Canvas.h>

void NQCanvas_release(NQCanvas* thiz)
{
  NQ_UNUSED_PARAM(thiz);
}

#endif /* CANVAS_BACKEND_STUB */
