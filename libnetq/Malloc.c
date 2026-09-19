/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/Malloc.h"

#include <libnetq/OS.h>

#if defined(NQ_OS_KERNEL)
# include <linux/slab.h>
#elif defined(NQ_OS_WINDOWS)
# include <windows.h>
#elif defined(NQ_OS_UNIX)
# include <libnetq/string/String.h>
#endif

void* NQMalloc(size_t size)
{
#if defined(NQ_OS_KERNEL)
  return kmalloc(size, GFP_KERNEL);

#elif defined(NQ_OS_WINDOWS)
  HANDLE heap = GetProcessHeap();
  if (NQ_LIKELY(heap))
    return HeapAlloc(heap, 0, size);
  return NULL;

#elif defined(NQ_OS_UNIX)
  return malloc(size);

#else
  return NULL;

#endif
}

void* NQZeroMalloc(size_t size)
{
#if defined(NQ_OS_KERNEL)
  return kzalloc(size, GFP_KERNEL);

#elif defined(NQ_OS_WINDOWS)
  HANDLE heap = GetProcessHeap();
  if (NQ_LIKELY(heap))
    return HeapAlloc(heap, HEAP_ZERO_MEMORY, size);
  return NULL;

#elif defined(NQ_OS_UNIX)
  void* ptr = malloc(size);
  if (NQ_LIKELY(ptr))
    memset(ptr, 0, size);
  return ptr;

#else
  return NULL;

#endif
}

void* NQCalloc(size_t n, size_t size)
{
#if defined(NQ_OS_KERNEL)
  return kmalloc_array(n, size, GFP_KERNEL | __GFP_ZERO);

#elif defined(NQ_OS_WINDOWS)
  HANDLE heap = GetProcessHeap();
  if (NQ_LIKELY(heap))
    return HeapAlloc(heap, HEAP_ZERO_MEMORY, n * size);
  return NULL;

#elif defined(NQ_OS_UNIX)
  return calloc(n, size);

#else
  return NULL;

#endif
}

void* NQRealloc(void* ptr, size_t size)
{
#if defined(NQ_OS_KERNEL)
  return krealloc(ptr, size, GFP_KERNEL);

#elif defined(NQ_OS_WINDOWS)
  HANDLE heap = GetProcessHeap();
  if (NQ_UNLIKELY(!heap))
    return NULL;

  if (NQ_UNLIKELY(!ptr))
    return HeapAlloc(heap, 0, size);

  if (NQ_UNLIKELY(!size)) {
    HeapFree(heap, 0, ptr);
    return NULL;
  }

  return HeapReAlloc(heap, 0, ptr, size);

#elif defined(NQ_OS_UNIX)
  return realloc(ptr, size);

#else
  return NULL;

#endif
}

void NQFree(void* ptr)
{
#if defined(NQ_OS_KERNEL)
  kfree(ptr);

#elif defined(NQ_OS_WINDOWS)
  HANDLE heap = GetProcessHeap();
  if (NQ_LIKELY(heap))
    HeapFree(heap, 0, ptr);

#elif defined(NQ_OS_UNIX)
  free(ptr);

#endif
}
