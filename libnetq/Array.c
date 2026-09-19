/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_CLASS_NAME "NQUint8Array"
#define NQ_LOG_TAG NQ_CLASS_NAME

#include "config.h"
#include "libnetq/Array.h"

#include <libnetq/String.h>
#include <libnetq/Malloc.h>
#include <libnetq/Limits.h>
#include <libnetq/fs/FileHandle.h>
#include <libnetq/Log.h>

static NQUint8Array s_zeroArray = { 0, {0} };

static void NQUint8Array_init(NQUint8Array* array, const uint8_t* data, uint32_t size)
{
  array->size = size;
  memcpy(array->data, data, size);
}

NQUint8Array* NQUint8Array_alloc(size_t size)
{
  if (NQ_UINT32_MAX < size)
    return NULL;

  if (size == 0)
    return &s_zeroArray;

  NQUint8Array* array = (NQUint8Array*)NQMalloc(sizeof(NQUint8Array) + size - 1);
  if (array == NULL)
    return NULL;

  array->size = (uint32_t)size;
  return array;
}

NQUint8Array* NQUint8Array_create(const uint8_t* data, size_t size)
{
  NQUint8Array* array = NQUint8Array_alloc(size);
  if (array == NULL)
    return NULL;

  memcpy(array->data, data, size);
  return array;
}

void NQUint8Array_destroy(NQUint8Array* array)
{
  if (array == &s_zeroArray)
    return;
  
  NQFree((void*)array);
}

NQUint8Array* NQUint8Array_copy(const NQUint8Array* array)
{
  if (array == &s_zeroArray)
    return &s_zeroArray;

  NQUint8Array* newArray = (NQUint8Array*)NQMalloc(sizeof(NQUint8Array) + array->size - 1);
  if (newArray == NULL)
    return NULL;

  NQUint8Array_init(newArray, array->data, array->size);
  return newArray;
}

NQUint8Array* NQUint8Array_fromFile(const char* filename)
{
  NQFileHandle handle;
  int ret = NQFileOpen(filename, NQ_FOPEN_READ, &handle);
  if (ret != 0) {
    NQ_LOGE("File '%s' returned error - %i", filename, -ret);
    return NULL;
  }

  long long size = NQFileGetSize(handle);
  if (size < 0) {
    NQ_LOGE("Error %i occurred while retrieving the size", (int)size);
    NQFileClose(handle);
    return NULL;
  }
  if (size > NQ_UINT32_MAX) {
    NQ_LOGE("File %s is too big", filename);
    NQFileClose(handle);
    return NULL;
  }

  NQUint8Array* thiz = NQUint8Array_alloc((uint32_t)size);
  if (thiz != NULL) {
    int64_t n = NQFileReadn(handle, NQUint8Array_data(thiz), (int64_t)size);
    if (n != (int64_t)size){
      NQUint8Array_destroy(thiz);
      thiz = NULL;
    }
  }

  NQFileClose(handle);
  return thiz;
}
