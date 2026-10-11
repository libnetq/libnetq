/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/fs/win32/Link.h"

#ifdef NQ_OS_WINDOWS

#include <libnetq/fs/Path.h>
#include <libnetq/string/String.h>
#include <libnetq/ErrorCode.h>

#include <windows.h>

#ifndef SYMBOLIC_LINK_FLAG_DIRECTORY
# define SYMBOLIC_LINK_FLAG_DIRECTORY 0x1
#endif
#ifndef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
# define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE 0x2
#endif

static bool IsAbsoluteWinPath(const NQWChar* path)
{
  return path[0] == L'\\' || (path[0] != L'\0' && path[1] == L':');
}

/*
 * Windows needs to know up front whether the link points to a directory.
 * A relative target is resolved against the directory containing the link,
 * not the current working directory.
 */
static bool IsTargetDirectory(const NQWinPath* target, const NQWinPath* link)
{
  NQWChar resolved[NQ_ARRAY_LENGTH(target->characters) * 2];
  const NQWChar* path = target->characters;

  if (!IsAbsoluteWinPath(target->characters)) {
    size_t dirLength = link->length;
    while (dirLength > 0 && link->characters[dirLength - 1] != L'\\')
      --dirLength;

    if (dirLength > 0 && dirLength + target->length < NQ_ARRAY_LENGTH(resolved)) {
      NQMemcpy(resolved, link->characters, dirLength * sizeof(NQWChar));
      NQMemcpy(resolved + dirLength, target->characters, (target->length + 1) * sizeof(NQWChar));
      path = resolved;
    }
  }

  DWORD attributes = GetFileAttributesW(path);
  return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

int NQSymlink(const char* target, const char* linkPath)
{
  if (!target || !*target || !linkPath || !*linkPath)
    return -NQ_EINVAL;

  NQWinPath wintarget;
  if (!NQWinPathInit(&wintarget, target))
    return -NQ_ENOMEM;

  NQWinPath winlink;
  if (!NQWinPathInit(&winlink, linkPath)) {
    NQWinPathFinalize(&wintarget);
    return -NQ_ENOMEM;
  }

  DWORD flags = SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
  if (IsTargetDirectory(&wintarget, &winlink))
    flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;

  BOOLEAN success = CreateSymbolicLinkW(winlink.characters, wintarget.characters, flags);
  /* Pre-Creators Update Windows rejects the unprivileged flag. */
  if (!success && GetLastError() == ERROR_INVALID_PARAMETER) {
    flags &= ~SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
    success = CreateSymbolicLinkW(winlink.characters, wintarget.characters, flags);
  }

  NQWinPathFinalize(&winlink);
  NQWinPathFinalize(&wintarget);
  return success ? 0 : -NQGetLastError();
}

int NQLink(const char* existingPath, const char* newPath)
{
  if (!existingPath || !*existingPath || !newPath || !*newPath)
    return -NQ_EINVAL;

  NQWinPath winexisting;
  if (!NQWinPathInit(&winexisting, existingPath))
    return -NQ_ENOMEM;

  NQWinPath winnew;
  if (!NQWinPathInit(&winnew, newPath)) {
    NQWinPathFinalize(&winexisting);
    return -NQ_ENOMEM;
  }

  WINBOOL success = CreateHardLinkW(winnew.characters, winexisting.characters, NULL);
  NQWinPathFinalize(&winnew);
  NQWinPathFinalize(&winexisting);
  return success ? 0 : -NQGetLastError();
}

#endif
