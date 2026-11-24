/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_LOG_TAG "NQGfxWindow"

#include "config.h"
#include "libnetq/gfx/GfxWindow.h"
#include "libnetq/Log.h"
#include "libnetq/Malloc.h"
#include "libnetq/input/WindowEvent.h"

NQGfxWindow* NQGfxWindow_create(const struct NQGfxWindowOperations* ops, const NQGfxWindowConfig* config)
{
  NQGfxWindow* thiz;

  if (!ops || !config) {
    NQ_LOGE("Missing %s", !ops ? "operations" : "config");
    return NULL;
  }
  if (ops->size < sizeof(NQGfxWindow)) {
    NQ_LOGE("Operations size %zu is smaller than NQGfxWindow (%zu)", ops->size, sizeof(NQGfxWindow));
    return NULL;
  }

  thiz = (NQGfxWindow*)NQZalloc(ops->size);
  if (!thiz) {
    NQ_LOGE("Unable to allocate window (%zu bytes)", ops->size);
    return NULL;
  }

  thiz->ops = ops;
  thiz->callback = config->callback;
  thiz->userdata = config->userdata;
  thiz->width = config->width;
  thiz->height = config->height;
  thiz->visible = false;
  thiz->focused = false;
  thiz->redrawPending = false;

  /* init cleans up after itself when it fails; destroy is not called then */
  if (!ops->init(thiz, config)) {
    NQ_LOGE("Window init failed (%dx%d)", config->width, config->height);
    NQFree(thiz);
    return NULL;
  }

  return thiz;
}

void NQGfxWindow_destroy(NQGfxWindow* thiz)
{
  thiz->ops->destroy(thiz);
  NQFree(thiz);
}

bool NQGfxWindow_dispatchEvent(NQGfxWindow* thiz, const NQEvent* event)
{
  switch (event->type) {
    case NQ_EVENT_WINDOW_RESIZED: {
      const NQWindowResizedEvent* resized = (const NQWindowResizedEvent*)event;
      thiz->width = resized->width;
      thiz->height = resized->height;
      break;
    }
    case NQ_EVENT_WINDOW_SHOW:
      thiz->visible = true;
      break;
    case NQ_EVENT_WINDOW_HIDE:
      thiz->visible = false;
      break;
    case NQ_EVENT_WINDOW_FOCUS_IN:
      thiz->focused = true;
      break;
    case NQ_EVENT_WINDOW_FOCUS_OUT:
      thiz->focused = false;
      break;
    case NQ_EVENT_WINDOW_DRAW:
      thiz->redrawPending = false;
      break;
    default:
      break;
  }

  return thiz->callback(thiz->userdata, event);
}

void NQGfxWindow_invalidate(NQGfxWindow* thiz)
{
  if (!thiz->redrawPending) {
    thiz->redrawPending = true;
    thiz->ops->invalidate(thiz);
  }
}
