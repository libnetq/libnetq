/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/gfx/nanovg/Canvas.h"

#ifdef CANVAS_BACKEND_NANOVG

#include <libnetq/gfx/Canvas.h>

#include <libnetq/Malloc.h>
#include <libnetq/string/String.h>

NQCanvas* NQCanvas_fromNanovg(NVGcontext* context)
{
  if (!context)
    return NULL;
  NQCanvas* thiz = (NQCanvas*)NQMalloc(sizeof(NQCanvas));
  if (!thiz)
    return NULL;
  thiz->context = context;
  thiz->text = (NQTextState){0};
  return thiz;
}

void NQCanvas_release(NQCanvas* thiz)
{
  if (!thiz)
    return;
  NQCanvasContext_release(thiz->context);
  NQFree(thiz);
}

#endif /* CANVAS_BACKEND_NANOVG */
