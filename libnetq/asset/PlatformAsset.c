/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_CLASS_NAME "NQFileSystemAsset"
#define NQ_LOG_TAG NQ_CLASS_NAME

#include "config.h"
#include "libnetq/asset/PlatformAsset.h"

#include <libnetq/Assert.h>

struct PlatformAsset {
  NQAsset base;
  NQAssetHandle handle;
};

#if defined(NQ_OS_ANDROID)

#include <libnetq/Malloc.h>

struct PlatformAssetDir {
  NQAssetDir base;
  AAssetDir* impl;
};

struct PlatformAssetFile {
  NQAssetFile base;
  AAsset* impl;
};

static void dirClose(NQAssetDir* dir)
{
  struct PlatformAssetDir* thiz = (struct PlatformAssetDir*)dir;
  AAssetDir_close(thiz->impl);
  NQFree(thiz);
}

static const char* dirReadFileName(NQAssetDir* dir)
{
  struct PlatformAssetDir* thiz = (struct PlatformAssetDir*)dir;
  return AAssetDir_getNextFileName(thiz->impl);
}

static const struct NQAssetDirCallbacks s_assetDirCallbacks =
{
  .close = dirClose,
  .readFileName = dirReadFileName,
};

NQAssetDir* NQPlatformAssetOpenDir(NQAssetHandle handle, const char* dirname)
{
  AAssetDir* impl = AAssetManager_openDir(handle, dirname);
  if (impl == NULL) {
    return NULL;
  }

  struct PlatformAssetDir* thiz = (struct PlatformAssetDir*)NQMalloc(sizeof(*thiz));
  if (thiz == NULL) {
    AAssetDir_close(impl);
    return NULL;
  }

  thiz->base.callbacks = &s_assetDirCallbacks;
  thiz->impl = impl;

  return &thiz->base;
}

static void fileClose(NQAssetFile* file)
{
  struct PlatformAssetFile* thiz = (struct PlatformAssetFile*)file;
  AAsset_close(thiz->impl);
  NQFree(thiz);
}

static int64_t fileGetSize(NQAssetFile* file)
{
  struct PlatformAssetFile* thiz = (struct PlatformAssetFile*)file;
#ifdef HAVE_AASSET_GETLENGTH64
  return (int64_t)AAsset_getLength64(thiz->impl);
#else
  // Try dlopen AAsset_getLength64
  return (int64_t)AAsset_getLength(thiz->impl);
#endif
}

static int fileRead(NQAssetFile* file, void* buffer, size_t size)
{
  struct PlatformAssetFile* thiz = (struct PlatformAssetFile*)file;
  return AAsset_read(thiz->impl, buffer, size);
}

static const struct NQAssetFileCallbacks s_fileCallbacks =
{
  .close = fileClose,
  .getSize = fileGetSize,
  .read = fileRead,
};

NQAssetFile* NQPlatformAssetOpenFile(NQAssetHandle handle, const char* filename, int mode)
{
  AAsset* impl = AAssetManager_open(handle, filename, AASSET_MODE_STREAMING);
  if (impl == NULL) {
    return NULL;
  }

  struct PlatformAssetFile* thiz = (struct PlatformAssetFile*)NQMalloc(sizeof(*thiz));
  if (thiz == NULL) {
    AAsset_close(impl);
    return NULL;
  }

  thiz->base.callbacks = &s_fileCallbacks;
  thiz->impl = impl;

  return &thiz->base;
}

#else

NQAssetDir* NQPlatformAssetOpenDir(NQAssetHandle handle, const char* dirname)
{
  return NULL;
}

NQAssetFile* NQPlatformAssetOpenFile(NQAssetHandle handle, const char* filename, int mode)
{
  return NULL;
}

#endif


static NQAssetDir* platformAssetOpenDir(NQAsset* asset, const char* dirname)
{
  struct PlatformAsset* thiz = NQ_CONTAINER_OF(asset, struct PlatformAsset, base);
  return NQPlatformAssetOpenDir(thiz->handle, dirname);
}

static NQAssetFile* platformAssetOpenFile(NQAsset* asset, const char* filename, int mode)
{
  struct PlatformAsset* thiz = NQ_CONTAINER_OF(asset, struct PlatformAsset, base);
  return NQPlatformAssetOpenFile(thiz->handle, filename, mode);
}

static void platformAssetDestroy(NQAsset* asset)
{
  struct PlatformAsset* thiz = NQ_CONTAINER_OF(asset, struct PlatformAsset, base);
  if (thiz->handle) {
#ifdef NQ_OS_MACOSX
    CFRelease(thiz->handle);
#endif
    thiz->handle = NULL;
  }
}

static const struct NQAssetCallbacks s_platformAssetCallbacks =
{
  .destroy = platformAssetDestroy,
  .openDir = platformAssetOpenDir,
  .openFile = platformAssetOpenFile,
};

static struct PlatformAsset s_platformAsset =
{
  .base = { &s_platformAssetCallbacks },
  .handle = NULL,
};

NQAsset* NQPlatformAssetGetInstance(void)
{
  return &s_platformAsset.base;
}

void NQPlatformAssetInit(NQAssetHandle handle)
{
#ifdef NQ_OS_MACOSX
  /* the bundle is kept; NULL (no bundle) is fine, as elsewhere */
  if (s_platformAsset.handle)
    CFRelease(s_platformAsset.handle);
  s_platformAsset.handle = handle ? (CFBundleRef)CFRetain(handle) : NULL;
#else
  s_platformAsset.handle = handle;
#endif
}
