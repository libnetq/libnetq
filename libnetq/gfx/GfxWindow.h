/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_GFX_GFXWINDOW_H
#define _LIBNETQ_GFX_GFXWINDOW_H

#include <libnetq/Basic.h>
#include <libnetq/Event.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQGfxWindow NQGfxWindow;
typedef struct NQGfxWindowConfig NQGfxWindowConfig;

enum {
  NQ_GFX_WINDOW_RESIZABLE  = 1 << 0,
  NQ_GFX_WINDOW_FULLSCREEN = 1 << 1,
};

struct NQGfxWindowConfig {
  const char* title;
  int width;              /* content size in logical units */
  int height;
  int minWidth;           /* 0 == no limit */
  int minHeight;
  uint32_t flags;         /* NQ_GFX_WINDOW_* */

  NQEventCallback* callback;  /* (userdata, event) -> handled */
  void* userdata;
};

/* Implemented by the backend. Every hook gets the embedded NQGfxWindow;
 * the backend uses NQ_CONTAINER_OF to reach its own structure. Optional
 * hooks may be NULL; the NQGfxWindow_* wrapper then does nothing (or
 * returns NQ_FALSE). Hooks are called only by GfxWindow.c, never directly. */
struct NQGfxWindowOperations {
  const char* name;       /* "x11", "wayland", "win32", "cocoa", "uikit", "android" */
  size_t size;            /* sizeof the backend structure embedding NQGfxWindow */
  bool resizable;

  bool (*init) (NQGfxWindow*, const NQGfxWindowConfig*);  /* create native window, hidden */
  void (*destroy) (NQGfxWindow*);                         /* release native resources, not the memory */
  void (*invalidate) (NQGfxWindow*);                      /* schedule an NQ_EVENT_WINDOW_DRAW */

  void (*show) (NQGfxWindow*);
  void (*hide) (NQGfxWindow*);
  void (*setTitle) (NQGfxWindow*, const char* title);
  void (*resizeTo) (NQGfxWindow*, int width, int height);
  void (*setMinSize) (NQGfxWindow*, int width, int height);
  void (*setFullscreen) (NQGfxWindow*, bool value);
  void (*showKeyboard) (NQGfxWindow*, bool value);        /* on-screen keyboard */
  void (*close) (NQGfxWindow*);                           /* ask to close (as the user would) */
};

/* Common part, owned by GfxWindow.c. Backends read it, and change it only
 * through NQGfxWindow_dispatchEvent. Everyone else treats it as opaque. */
struct NQGfxWindow {
  const struct NQGfxWindowOperations* ops;

  NQEventCallback* callback;  /* (userdata, event) -> handled */
  void* userdata;

  int width;                  /* content size, logical units */
  int height;
  bool visible;
  bool focused;
  bool redrawPending;         /* invalidate called, DRAW not yet delivered */
};

NQ_EXPORT NQGfxWindow* NQGfxWindow_create(const struct NQGfxWindowOperations* ops, const NQGfxWindowConfig* config);
NQ_EXPORT void NQGfxWindow_destroy(NQGfxWindow*);

NQ_EXPORT bool NQGfxWindow_dispatchEvent(NQGfxWindow*, const NQEvent* event);
NQ_EXPORT void NQGfxWindow_invalidate(NQGfxWindow* thiz);

static inline int NQGfxWindow_width(const NQGfxWindow* thiz)
{
  return thiz->width;
}

static inline int NQGfxWindow_height(const NQGfxWindow* thiz)
{
  return thiz->height;
}

static inline bool NQGfxWindow_isVisible(const NQGfxWindow* thiz)
{
  return thiz->visible;
}

static inline bool NQGfxWindow_isFocused(const NQGfxWindow* thiz)
{
  return thiz->focused;
}

static inline const char* NQGfxWindow_name(const NQGfxWindow* thiz)
{
  return thiz->ops->name;
}

static inline bool NQGfxWindow_resizable(const NQGfxWindow* thiz)
{
  return thiz->ops->resizable;
}

static inline void NQGfxWindow_show(NQGfxWindow* thiz)
{
  thiz->ops->show(thiz);
}

static inline void NQGfxWindow_hide(NQGfxWindow* thiz)
{
  thiz->ops->hide(thiz);
}

static inline void NQGfxWindow_setTitle(NQGfxWindow* thiz, const char* title)
{
  thiz->ops->setTitle(thiz, title);
}

static inline void NQGfxWindow_resizeTo(NQGfxWindow* thiz, int width, int height)
{
  thiz->ops->resizeTo(thiz, width, height);
}

static inline void NQGfxWindow_setMinSize(NQGfxWindow* thiz, int width, int height)
{
  thiz->ops->setMinSize(thiz, width, height);
}

static inline void NQGfxWindow_setFullscreen(NQGfxWindow* thiz, bool value)
{
  thiz->ops->setFullscreen(thiz, value);
}

static inline void NQGfxWindow_showKeyboard(NQGfxWindow* thiz, bool value)
{
  thiz->ops->showKeyboard(thiz, value);
}

static inline void NQGfxWindow_close(NQGfxWindow* thiz)
{
  thiz->ops->close(thiz);
}

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_GFX_GFXWINDOW_H */
