/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_CLASS_NAME "NQAsset"
#define NQ_LOG_TAG NQ_CLASS_NAME

#include "config.h"
#include "libnetq/Asset.h"

#include <libnetq/string/StringUtil.h>
#include <libnetq/Malloc.h>
#include <libnetq/asset/PlatformAsset.h>
#include <libnetq/asset/BinaryAsset.h>
#include <libnetq/asset/FileSystemAsset.h>

static NQAsset* s_assets[] =
{
  NULL,
  NULL,
  NULL,
  NULL,
};

void NQAsset_destroy(NQAsset* thiz)
{
  thiz->callbacks->destroy(thiz);
}

NQAssetDir* NQAsset_openDir(NQAsset* thiz, const char* dirname)
{
  return thiz->callbacks->openDir(thiz, dirname);
}

NQAssetFile* NQAsset_openFile(NQAsset* thiz, const char* filename, int mode)
{
  return thiz->callbacks->openFile(thiz, filename, mode);
}

NQUint8Array* NQAssetFile_loadBytes(NQAsset* thiz, const char* filename)
{
  NQAssetFile* file = NQAsset_openFile(thiz, filename, 0);
  if (file == NULL) {
    return NULL;
  }

  int64_t size = NQAssetFile_getSize(file);
  NQUint8Array* bytes = NQUint8Array_alloc((uint32_t)size);
  if (bytes == NULL) {
    NQAssetFile_close(file);
    return NULL;
  }

  int64_t n = NQAssetFile_read(file, NQUint8Array_data(bytes), NQUint8Array_size(bytes));
  if (n != size){
    NQUint8Array_destroy(bytes);
    bytes = NULL;
  }

  NQAssetFile_close(file);
  return bytes;
}

void NQAssetInitialize(const void* binary, const char* dirname, NQAssetHandle handle)
{
  NQPlatformAssetInit(handle);

  NQAsset* asset;
  NQAsset** pAsset = s_assets;

  uint32_t size = 0;
  const uint8_t* data = NULL;

  if (binary != NULL && !NQMemcmp(binary, "AST", 3)) {
    data = (uint8_t*)binary + 4;
    NQMemcpy(&size, data, sizeof(size));
    data += sizeof(size);
    if (data && size) {
      asset = NQBinaryAssetCreate(data, size);
      if (asset)
        *pAsset++ = asset;
    }
  }

  if (dirname) {
    asset = NQFileSystemAssetCreate(dirname);
    if (asset)
      *pAsset++ = asset;
  }

  asset = NQPlatformAssetGetInstance();
  if (asset)
    *pAsset++ = asset;
}

void NQAssetShutdown(void)
{
  NQAsset** pAsset = s_assets;

  while (*pAsset) {
    NQAsset_destroy(*pAsset);
    *pAsset++ = NULL;
  }
}

NQAssetDir* NQAssetDir_open(const char* dirname)
{
  for (size_t i = 0; i < NQ_ARRAY_LENGTH(s_assets); i++) {
    NQAsset* asset = s_assets[i];
    if (!asset)
      return NULL;

    NQAssetDir* dir = NQAsset_openDir(asset, dirname);
    if (dir)
      return dir;
  }

  return NULL;
}

void NQAssetDir_close(NQAssetDir* thiz)
{
  thiz->callbacks->close(thiz);
}

const char* NQAssetDir_readFileName(NQAssetDir* thiz)
{
  return thiz->callbacks->readFileName(thiz);
}

NQAssetFile* NQAssetFile_open(const char* filename, int mode)
{
  for (size_t i = 0; i < NQ_ARRAY_LENGTH(s_assets); i++) {
    NQAsset* asset = s_assets[i];
    if (!asset)
      return NULL;

    NQAssetFile* file = NQAsset_openFile(asset, filename, mode);
    if (file)
      return file;
  }

  return NULL;
}

void NQAssetFile_close(NQAssetFile* thiz)
{
  thiz->callbacks->close(thiz);
}

int64_t NQAssetFile_getSize(NQAssetFile* thiz)
{
  return thiz->callbacks->getSize(thiz);
}

int NQAssetFile_read(NQAssetFile* thiz, void* buffer, size_t size)
{
  return thiz->callbacks->read(thiz, buffer, size);
}
