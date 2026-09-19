/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_FILESYSTEM_H
#define _LIBNETQ_FS_FILESYSTEM_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

NQ_EXPORT int64_t NQReadFile(const char* path, uint8_t* data, int64_t size);
NQ_EXPORT int64_t NQWriteFile(const char* path, const uint8_t* data, int64_t size);
NQ_EXPORT bool NQMakeDirectory(const char* path, bool recursive);
NQ_EXPORT bool NQFileExists(const char* path);
NQ_EXPORT bool NQDirectoryExists(const char* path);
NQ_EXPORT int NQRemoveFile(const char* path);

NQ_EXPORT int NQGetLogicalDrives(void);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_FS_FILESYSTEM_H */
