/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/fs/Statfs.h"

#ifdef NQ_OS_WINDOWS

#include <libnetq/ErrorCode.h>
#include <libnetq/fs/Path.h>
#include <libnetq/string/String.h>

#include <windows.h>

int NQGetStatfs(const char* path, NQStatfs* st)
{
  NQMemset(st, 0, sizeof(*st));

  NQWinPath winpath;
  if (!NQWinPathInit(&winpath, path))
    return -NQ_ENAMETOOLONG;
  /* GetVolumePathName takes names that do not exist */
  if (GetFileAttributesW(winpath.characters) == INVALID_FILE_ATTRIBUTES) {
    int result = -NQGetLastError();
    NQWinPathFinalize(&winpath);
    return result;
  }

  WCHAR root[MAX_PATH + 1];
  BOOL ok = GetVolumePathNameW(winpath.characters, root, MAX_PATH + 1);
  NQWinPathFinalize(&winpath);
  if (!ok)
    return -NQGetLastError();

  /* GetDiskFreeSpaceEx wants a directory: the volume's root */
  DWORD sectorsPerCluster, bytesPerSector, freeClusters, totalClusters;
  ULARGE_INTEGER available, total, totalFree;
  if (!GetDiskFreeSpaceW(root, &sectorsPerCluster, &bytesPerSector, &freeClusters, &totalClusters) ||
      !GetDiskFreeSpaceExW(root, &available, &total, &totalFree))
    return -NQGetLastError();

  uint64_t cluster = (uint64_t)sectorsPerCluster * bytesPerSector;
  if (cluster == 0)
    cluster = 4096;
  st->bsize = cluster;
  st->blocks = total.QuadPart / cluster;
  st->bfree = totalFree.QuadPart / cluster;
  st->bavail = available.QuadPart / cluster;

  DWORD serial = 0, nameMax = 0, fsFlags = 0;
  WCHAR fsName[MAX_PATH + 1];
  if (GetVolumeInformationW(root, NULL, 0, &serial, &nameMax, &fsFlags, fsName, MAX_PATH + 1)) {
    st->fsid = serial;
    st->namelen = nameMax;
    /* a name that does not fit is left out rather than cut */
    if (WideCharToMultiByte(CP_UTF8, 0, fsName, -1, st->typeStr, (int)sizeof(st->typeStr), NULL, NULL) <= 0)
      st->typeStr[0] = '\0';
  } else {
    st->namelen = 255;
  }
  return 0;
}

#endif /* NQ_OS_WINDOWS */
