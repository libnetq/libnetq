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

#include <libnetq/Event.h>
#include <libnetq/RefCount.h>

typedef struct NQGfxObject NQGfxObject;

struct NQGfxObjectOperations {
  const char* name;
  size_t size;
  void (*init) (NQGfxObject*);
  void (*event) (NQGfxObject*, const NQEvent* event);
  void (*destroy) (NQGfxObject*);
};

struct NQGfxObject {
  const struct NQGfxObjectOperations* operations;
  NQRefCount refCount;
};

struct NQGfxControl {
};

struct NQGfxWindow {
};

#endif /* _LIBNETQ_GFX_GFXWINDOW_H */
