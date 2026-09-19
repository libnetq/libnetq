/*
 * MIT License
 *
 * Copyright (c) 2024-2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_TARBALL_H
#define _LIBNETQ_TARBALL_H

#include <libnetq/Basic.h>
#include <libnetq/http/MediaType.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_TAR_MAGIC "ustar "
#define NQ_TAR_MIME NQ_MEDIATYPE_APPLICATION_XTAR

#define NQ_TAR_BLOCK_LEN (512)
#define NQ_TAR_FILENAME_LEN (100)
#define NQ_TAR_MODE_LEN (8)
#define NQ_TAR_UID_LEN (8)
#define NQ_TAR_GID_LEN (8)
#define NQ_TAR_SIZE_LEN (12)
#define NQ_TAR_MTIME_LEN (12)
#define NQ_TAR_CHKSUM_LEN (8)
#define NQ_TAR_TYPEFLAG_LEN (1)
#define NQ_TAR_LINKNAME_LEN (100)
#define NQ_TAR_MAGIC_LEN (6)
#define NQ_TAR_VERSION_LEN (2)
#define NQ_TAR_USERNAME_LEN (32)
#define NQ_TAR_GROUPNAME_LEN (32)
#define NQ_TAR_DEVMAJOR_LEN (8)
#define NQ_TAR_DEVMINOR_LEN (8)
#define NQ_TAR_PREFIX_LEN (155)

typedef struct NQTarball NQTarball;
typedef struct NQTarFileInfo NQTarFileInfo;

typedef bool (NQTarWriteCallback) (void* userdata, const void* data, size_t size);

enum NQTarTypeFlag {
  kNQTarRegularType = '0',
  kNQTarLinkType = '1',
  kNQTarSymbolicType = '2',
  kNQTarCharacterType = '3',
  kNQTarBlockType = '4',
  kNQTarDirectoryType = '5',
  kNQTarFIFOType = '6',
  kNQTarReservedType = '7',
};

struct NQTarFileInfo {
  const char* filename;
  const char* username;
  const char* groupname;
  uint32_t mode;
  uint32_t uid;
  uint32_t gid;
  uint64_t mtime;
  uint64_t atime;
  uint64_t ctime;
};

struct NQTarball {
  void* userdata;
  NQTarWriteCallback* callback;
  unsigned state;
};

NQ_EXPORT void NQTarball_init(NQTarball*, NQTarWriteCallback* callback, void* userdata);
NQ_EXPORT void NQTarball_finalize(NQTarball*);

NQ_EXPORT bool NQTarball_append(NQTarball*, const NQTarFileInfo* info, const void* data, size_t size);
NQ_EXPORT bool NQTarball_finish(NQTarball*);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_TARBALL_H */
