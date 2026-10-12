/*
 * MIT License
 *
 * Copyright (c) 2024-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/Tarball.h"

#include <libnetq/string/StringUtil.h>
#include <libnetq/Math.h>
#include <libnetq/Assert.h>
#include <libnetq/Time.h>

enum {
  kInitSTate,
};

void NQTarball_init(NQTarball* thiz, NQTarWriteCallback* callback, void* userdata)
{
  thiz->callback = callback;
  thiz->userdata = userdata;
  thiz->state = kInitSTate;
}

void NQTarball_finalize(NQTarball* thiz)
{
}

static NQ_ALWAYS_INLINE bool NQTarball_write(NQTarball* thiz, const void* data, size_t size)
{
  return thiz->callback(thiz->userdata, data, size);
}

static void writeUint64(uint64_t value, char* buffer, size_t size)
{
  NQ_ASSERT(size != 0);

  char* ptr = buffer + size;
  *--ptr = '\0';

  for (size_t i = 1; i < size; i++) {
    *--ptr = (value & 7) + '0';
    value >>= 3;
  }
}

bool NQTarball_append(NQTarball* thiz, const NQTarFileInfo* info, const void* data, size_t size)
{
  char block[NQ_TAR_BLOCK_LEN];
  NQMemset(block, 0, sizeof(block));

  char* ptr = block;

  size_t filenameLength = NQStrlen(info->filename);
  NQMemcpy(ptr, info->filename, NQGetMin(filenameLength, NQ_TAR_FILENAME_LEN));
  ptr += NQ_TAR_FILENAME_LEN;

  writeUint64(info->mode, ptr, NQ_TAR_MODE_LEN);
  ptr += NQ_TAR_MODE_LEN;

  writeUint64(info->uid, ptr, NQ_TAR_UID_LEN);
  ptr += NQ_TAR_UID_LEN;

  writeUint64(info->gid, ptr, NQ_TAR_GID_LEN);
  ptr += NQ_TAR_GID_LEN;

  writeUint64(size, ptr, NQ_TAR_SIZE_LEN);
  ptr += NQ_TAR_SIZE_LEN;

  writeUint64(info->mtime, ptr, NQ_TAR_MTIME_LEN);
  ptr += NQ_TAR_MTIME_LEN;

  char* chksumPtr = ptr;
  NQMemset(ptr, ' ', NQ_TAR_CHKSUM_LEN);
  ptr += NQ_TAR_CHKSUM_LEN;

  *ptr++ = kNQTarRegularType;
  ptr += NQ_TAR_LINKNAME_LEN;

  NQMemcpy(ptr, NQ_TAR_MAGIC, NQ_TAR_MAGIC_LEN);
  ptr += NQ_TAR_MAGIC_LEN;

  *ptr++ = ' ';
  *ptr++ = '\0';

  size_t usernameLength = NQStrlen(info->username);
  NQMemcpy(ptr, info->username, NQGetMin(usernameLength, NQ_TAR_USERNAME_LEN));
  ptr += NQ_TAR_USERNAME_LEN;

  size_t groupnameLength = NQStrlen(info->groupname);
  NQMemcpy(ptr, info->groupname, NQGetMin(groupnameLength, NQ_TAR_GROUPNAME_LEN));
  ptr += NQ_TAR_GROUPNAME_LEN;

  ptr += NQ_TAR_DEVMAJOR_LEN;
  ptr += NQ_TAR_DEVMINOR_LEN;
  ptr += NQ_TAR_PREFIX_LEN;

  uint32_t chksum = 0;
  for (size_t i = 0; i < sizeof(block); i++) {
    chksum += block[i];
  }

  writeUint64(chksum, chksumPtr, NQ_TAR_CHKSUM_LEN - 1);

  if (!NQTarball_write(thiz, block, sizeof(block))) {
    return false;
  }

  if (!NQTarball_write(thiz, data, size)) {
    return false;
  }

  size_t totalSize = (size + NQ_TAR_BLOCK_LEN) / NQ_TAR_BLOCK_LEN * NQ_TAR_BLOCK_LEN;
  size_t padSize = totalSize - size;
  if (padSize) {
    NQMemset(block, 0, padSize);
    if (!NQTarball_write(thiz, block, padSize)) {
      return false;
    }
  }

  return true;
}

bool NQTarball_finish(NQTarball* thiz)
{
  char block[NQ_TAR_BLOCK_LEN];
  NQMemset(block, 0, sizeof(block));

  if (!NQTarball_write(thiz, block, sizeof(block))) {
    return false;
  }

  if (!NQTarball_write(thiz, block, sizeof(block))) {
    return false;
  }

  return true;
}
