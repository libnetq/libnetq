/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/fs/win32/Stat.h"

#ifdef NQ_OS_WINDOWS

#include <libnetq/fs/Path.h>
#include <libnetq/Time.h>
#include <libnetq/Assert.h>
#include <libnetq/ErrorCode.h>

#include <windows.h>

static void FileTimeToSpec(FILETIME ft, NQTimeSpec* ts)
{
  NQWinFileTimeToTimeSpec(ft.dwHighDateTime, ft.dwLowDateTime, ts);
}

static uint32_t ModeOf(DWORD attributes, bool followLinks)
{
  uint32_t mode = (attributes & FILE_ATTRIBUTE_DIRECTORY) ? NQ_STAT_IFDIR : NQ_STAT_IFREG;
  if (!followLinks && (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
    mode |= NQ_STAT_IFLNK;
  return mode;
}

/* What can be had without opening the file: without NQ_STAT_EXTRA, and
 * for files that refuse even that (a page file in use). No links, identity or
 * allocation, and a reparse point describes itself: *attributes tells
 * whether it is one. */
static int GetStatByAttributes(const NQWChar* path, bool followLinks, NQStat* st, DWORD* attributes)
{
  WIN32_FILE_ATTRIBUTE_DATA data;
  if (!GetFileAttributesExW(path, GetFileExInfoStandard, &data))
    return -NQGetLastError();
  *attributes = data.dwFileAttributes;

  st->mode = ModeOf(data.dwFileAttributes, followLinks);
  st->size = ((uint64_t)data.nFileSizeHigh << 32) | (uint64_t)data.nFileSizeLow;
  FileTimeToSpec(data.ftLastAccessTime, &st->atime);
  FileTimeToSpec(data.ftLastWriteTime, &st->mtime);
  FileTimeToSpec(data.ftCreationTime, &st->ctime);
  st->blocks = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 0 : (st->size + 511) / 512;
  st->nlink = 1;
  st->dev = 0;
  st->ino = 0;
  return 0;
}

int NQGetStat(const char* path, unsigned flags, NQStat* st)
{
  NQ_ASSERT(st);
  bool followLinks = (flags & NQ_STAT_FOLLOW) != 0;

  NQWinPath winpath;
  if (!NQWinPathInit(&winpath, path))
    return -NQ_ENOMEM;

  DWORD attributes;
  if (!(flags & NQ_STAT_EXTRA)) {
    /* one call, unless a link is to be followed, which it cannot do */
    int result = GetStatByAttributes(winpath.characters, followLinks, st, &attributes);
    if (result != 0 || !followLinks || !(attributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
      NQWinPathFinalize(&winpath);
      return result;
    }
  }

  /* attributes only, which needs no access to the file's data */
  DWORD openFlags = FILE_FLAG_BACKUP_SEMANTICS | (followLinks ? 0 : FILE_FLAG_OPEN_REPARSE_POINT);
  HANDLE handle = CreateFileW(winpath.characters, FILE_READ_ATTRIBUTES,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                              OPEN_EXISTING, openFlags, NULL);
  if (handle == INVALID_HANDLE_VALUE) {
    int error = NQGetLastError();
    int result = error == ERROR_SHARING_VIOLATION || error == ERROR_ACCESS_DENIED
                   ? GetStatByAttributes(winpath.characters, followLinks, st, &attributes)
                   : -(int)error;
    NQWinPathFinalize(&winpath);
    return result;
  }
  NQWinPathFinalize(&winpath);

  BY_HANDLE_FILE_INFORMATION info;
  FILE_STANDARD_INFO standard;
  if (!GetFileInformationByHandle(handle, &info) ||
      !GetFileInformationByHandleEx(handle, FileStandardInfo, &standard, sizeof(standard))) {
    int result = -NQGetLastError();
    CloseHandle(handle);
    return result;
  }

  uint64_t allocated = (uint64_t)standard.AllocationSize.QuadPart;
  /* AllocationSize does not count what compression saves, nor the holes
   * of a sparse file on every file system; Cygwin goes by the compressed
   * size for both */
  FILE_COMPRESSION_INFO compression;
  if ((info.dwFileAttributes & (FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_SPARSE_FILE)) &&
      GetFileInformationByHandleEx(handle, FileCompressionInfo, &compression, sizeof(compression)) &&
      (uint64_t)compression.CompressedFileSize.QuadPart < allocated) {
    allocated = (uint64_t)compression.CompressedFileSize.QuadPart;
  }
  CloseHandle(handle);

  st->mode = ModeOf(info.dwFileAttributes, followLinks);
  st->size = ((uint64_t)info.nFileSizeHigh << 32) | (uint64_t)info.nFileSizeLow;
  FileTimeToSpec(info.ftLastAccessTime, &st->atime);
  FileTimeToSpec(info.ftLastWriteTime, &st->mtime);
  FileTimeToSpec(info.ftCreationTime, &st->ctime);
  st->blocks = (allocated + 511) / 512;
  st->nlink = info.nNumberOfLinks;
  st->dev = info.dwVolumeSerialNumber;
  st->ino = ((uint64_t)info.nFileIndexHigh << 32) | (uint64_t)info.nFileIndexLow;

  return 0;
}

#endif
