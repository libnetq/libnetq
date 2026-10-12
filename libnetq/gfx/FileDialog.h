/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FILEDIALOG_H
#define _LIBNETQ_FILEDIALOG_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_FILE_DIALOG_DIRECTORY 0x01

typedef struct NQFileDialogOptions {
  unsigned flags;
  const char* title;
  const char* initialPath;
  uintptr_t parent;
  const char* parentHandle;
  int64_t* processId;
} NQFileDialogOptions;

typedef void (*NQFileDialogCallback)(void* userdata, const char* path, int error);

NQ_EXPORT int NQFileDialogOpen(const NQFileDialogOptions* options, NQFileDialogCallback callback, void* userdata);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_FILEDIALOG_H */
