/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_PROCESS_SHELLOPEN_H
#define _LIBNETQ_PROCESS_SHELLOPEN_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opening files, folders and URLs the way the desktop does when they are
 * double-clicked, as macOS's open(1): with the application the system has
 * for each, a folder in the file manager.
 *
 *   Windows  ShellExecuteEx (WITH_SHELL32, else -NQ_ENOSYS); the file is
 *            shown in Explorer with SHOpenFolderAndSelectItems (WITH_OLE32,
 *            else "explorer /select,"); the text editor is the .txt one
 *            (WITH_SHLWAPI, else Notepad).
 *   macOS    Launch Services (WITH_CORESERVICES, else -NQ_ENOSYS); the file
 *            is shown in the Finder by NSWorkspace.
 *   Linux,   xdg-open (or gio, kde-open, exo-open); applications from their
 *   BSD      desktop files and mimeapps.list; the file is shown in the file
 *            manager by org.freedesktop.FileManager1 over D-Bus, or its
 *            folder is opened.
 *   Others   (Android, iOS) -NQ_ENOSYS. */

/* Show the items selected in the file manager instead of opening them */
#define NQ_SHELL_OPEN_REVEAL       0x01
/* Open them with the default text editor: the application for text/plain
 * (public.plain-text, .txt) */
#define NQ_SHELL_OPEN_TEXT_EDITOR  0x02
/* Leave the application behind the others (macOS, Windows) */
#define NQ_SHELL_OPEN_BACKGROUND   0x04
/* Start another instance of the application even if it runs (macOS) */
#define NQ_SHELL_OPEN_NEW_INSTANCE 0x08
/* Start the application hidden (macOS), minimized (Windows) */
#define NQ_SHELL_OPEN_HIDE         0x10
/* Start the application without restoring its windows (macOS) */
#define NQ_SHELL_OPEN_FRESH        0x20
/* Give back an NQShellOpenWait for the applications started */
#define NQ_SHELL_OPEN_WAIT         0x40

typedef struct NQShellOpenOptions {
  unsigned flags; /* NQ_SHELL_OPEN_* */
  /* The application to open the items with, instead of each one's: a
   * path to it (an .app bundle, an executable, a .desktop file) or its
   * name as the system knows it ("Safari", "notepad", "firefox", a
   * desktop file's Name); NULL for the default. */
  const char* application;
  /* Or the application's identifier: a bundle identifier on macOS, a
   * desktop file ID on Linux, an AppUserModelID on Windows */
  const char* bundleId;
  /* Arguments for the application, after the items. They reach only an
   * application that is started, not one handed the items while it runs;
   * on Linux and Windows only one given as above. */
  const char* const* args;
  size_t argCount;
} NQShellOpenOptions;

/* What NQ_SHELL_OPEN_WAIT gives: the processes started, to wait for */
typedef struct NQShellOpenWait NQShellOpenWait;

/* Opens `count` items: absolute paths or URLs ("https://...", "mailto:...").
 * With an application and no items, starts it. `options` may be NULL for
 * the defaults; `wait` may be NULL unless NQ_SHELL_OPEN_WAIT is given, when it
 * is set to what to wait for (NULL if nothing was started that can be).
 *
 * Returns 0, or a negative NQ error code:
 *   -NQ_ENOENT    the application was not found
 *   -NQ_ENOTSUPP  no application opens the item, or the system cannot do
 *                 what the flags ask
 *   -NQ_ENOSYS    there is no desktop to open things on (on Linux and the
 *                 BSDs: no xdg-open, gio, kde-open or exo-open)
 * Linux and the BSDs hand the item to xdg-open and do not see whether an
 * application took it, unless NQ_SHELL_OPEN_WAIT waits for xdg-open itself. */
NQ_EXPORT int NQShellOpen(const char* const* items, size_t count, const NQShellOpenOptions* options, NQShellOpenWait** wait);

/* NQShellOpen of one item with the default application */
NQ_EXPORT int NQShellOpenItem(const char* item);

/* Returns 1 while any of the processes runs, 0 once all have ended. Does
 * not block: call it again later. With the item handed to xdg-open on
 * Linux, that is what is waited for, not the application it chose. */
NQ_EXPORT int NQShellOpenWait_poll(NQShellOpenWait* wait);
NQ_EXPORT void NQShellOpenWait_free(NQShellOpenWait* wait);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_PROCESS_SHELLOPEN_H */
