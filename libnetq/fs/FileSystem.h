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
#include <libnetq/Time.h>

#ifdef __cplusplus
extern "C" {
#endif

NQ_EXPORT int64_t NQReadFile(const char* path, uint8_t* data, int64_t size);
NQ_EXPORT int64_t NQWriteFile(const char* path, const uint8_t* data, int64_t size);
NQ_EXPORT bool NQMakeDirectory(const char* path, bool recursive);
NQ_EXPORT bool NQFileExists(const char* path);
NQ_EXPORT bool NQDirectoryExists(const char* path);
NQ_EXPORT int NQRemoveFile(const char* path);
NQ_EXPORT int NQRemoveDirectory(const char* path);
NQ_EXPORT int NQRename(const char* oldPath, const char* newPath);

/* Resolves `path` to an absolute path with no ".", ".." or symbolic links,
 * as realpath(3) does: every component must exist. The result uses '/'
 * separators on Windows too. Returns its length, or a negative NQ error
 * code (-NQ_ERANGE when it does not fit in `n` bytes). */
NQ_EXPORT int NQRealPath(char* buffer, size_t n, const char* path);
NQ_EXPORT int NQSetFileMode(const char* path, unsigned mode);
NQ_EXPORT int NQSetFileTimes(const char* path, NQTimeMs accessTime, NQTimeMs modificationTime);
NQ_EXPORT unsigned NQGetUmask(void);

#define NQ_ACCESS_EXISTS 0
#define NQ_ACCESS_READ   4
#define NQ_ACCESS_WRITE  2
#define NQ_ACCESS_EXEC   1

NQ_EXPORT int NQCheckAccess(const char* path, int mode);

/* tv_nsec values for NQSetFileTimeSpecs, as UTIME_NOW / UTIME_OMIT */
#define NQ_FILE_TIME_NOW  0x3fffffffL
#define NQ_FILE_TIME_OMIT 0x3ffffffeL

NQ_EXPORT int NQSetFileTimeSpecs(const char* path, const NQTimeSpec times[2], bool followLinks);

/* Owner and group values for NQSetFileOwner: NQ_OWNER_KEEP leaves one as it is */
#define NQ_OWNER_KEEP ((unsigned)-1)

/* Gives `path` the user and group IDs `owner` and `group`, as chown()
 * does, or lchown() for a symbolic link itself when followLinks is false.
 * Returns 0 or a negative error code; -NQ_ENOTSUPP where files have no
 * such IDs (Windows). */
NQ_EXPORT int NQSetFileOwner(const char* path, unsigned owner, unsigned group, bool followLinks);

NQ_EXPORT int NQGetLogicalDrives(void);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_FS_FILESYSTEM_H */
