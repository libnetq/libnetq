/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/Library.h"

#include <libnetq/fs/Path.h>
#include <libnetq/Assert.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/Log.h>

#if defined(NQ_OS_WINDOWS)
#include <windows.h>
#ifdef WITH_PSAPI
#include <psapi.h>
#endif
#include <libnetq/Malloc.h>
#include <libnetq/UTF.h>
#elif defined(NQ_OS_UNIX)
#include <dlfcn.h>
#endif

#if defined(NQ_OS_WINDOWS)
static NQLibrary openWinLibraryA(const char* path)
{
  NQLibrary handle = LoadLibraryExA(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
  return handle != NULL ? LoadLibraryExA(path, NULL, 0) : handle;
}

static NQLibrary openWinLibraryW(const WCHAR* path)
{
  NQLibrary handle = LoadLibraryExW(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
  return handle != NULL ? LoadLibraryExW(path, NULL, 0) : handle;
}
#endif

NQLibrary NQLibraryOpen(const char* path)
{
#if defined(NQ_OS_WINDOWS)
  WCHAR winpath[MAX_PATH];
  NQWinPathFrom(winpath, MAX_PATH, path);
  NQLibrary handle = openWinLibraryW(winpath);
  if (handle == NULL)
    handle = openWinLibraryA(path);
  return handle;
#elif defined(NQ_OS_UNIX)
  return dlopen(path, RTLD_LAZY);
#else
  return NULL;
#endif
}

void NQLibraryClose(NQLibrary handle)
{
#if defined(NQ_OS_WINDOWS)
  FreeLibrary(handle);
#elif defined(NQ_OS_UNIX)
  dlclose(handle);
#endif
}

NQSymbol NQLibraryGetSymbol(NQLibrary handle, const char* name)
{
#if defined(NQ_OS_WINDOWS)
  return GetProcAddress(handle, name);
#elif defined(NQ_OS_UNIX)
  return dlsym(handle, name);
#else
  return NULL;
#endif
}

int NQLibraryInfoLoad(NQLibraryInfo* info, const void* addr)
{
#if defined(NQ_OS_WINDOWS)
  HMODULE hModule = NULL;
  DWORD dwFlags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
  if (!GetModuleHandleExW(dwFlags, (LPCWSTR)addr, &hModule))
    return -(int)GetLastError();

#ifdef WITH_PSAPI
  MODULEINFO modInfo = {0};
  HANDLE hProcess = GetCurrentProcess();
  if (GetModuleInformation(hProcess, hModule, &modInfo, sizeof(modInfo)))
    info->baseAddr = modInfo.lpBaseOfDll;
  else
    info->baseAddr = NULL;
#else
  info->baseAddr = NULL;
#endif

  WCHAR winpath[MAX_PATH];
  DWORD fnSize = GetModuleFileNameW(hModule, winpath, MAX_PATH);
  if (fnSize == 0)
    return -(int)GetLastError();
  if (fnSize >= MAX_PATH)
    return -NQ_ENAMETOOLONG;

  NQUnicodeInfo fnInfo;
  if (!NQConvertUTF16ToUTF8((uint16_t*)winpath, (uint16_t*)winpath + fnSize, NULL, NULL, &fnInfo))
    return -NQ_EIO;

  char* filename = NQMalloc(fnInfo.utf8Size + 1);
  if (filename == NULL)
    return -NQ_ENOMEM;

  if (!NQConvertUTF16ToUTF8((uint16_t*)winpath, (uint16_t*)winpath + fnSize, (uint8_t*)filename, (uint8_t*)filename + fnInfo.utf8Size + 1, NULL)) {
    NQFree(filename);
    return -NQ_EIO;
  }
  NQ_ASSERT(filename[fnInfo.utf8Size] == '\0');

  for (size_t i = 0; i < fnInfo.utf8Size; i++) {
    if (filename[i] == NQ_WIN32_PATH_SEPARATOR)
      filename[i] = NQ_PATH_SEPARATOR;
  }

  info->filename = filename;
  return 0;

#elif defined(NQ_OS_UNIX)
  Dl_info dli;
  if (dladdr((void*)addr, &dli) == 0) {
    NQ_LOGE("dladdr(%p) failed: %s", addr, dlerror());
    return -NQ_EIO;
  }

  info->filename = dli.dli_fname;
  info->baseAddr = dli.dli_fbase;
  return 0;

#else
  return -NQ_ENOTSUPP;
#endif
}

void NQLibraryInfoFinalize(NQLibraryInfo* info)
{
#if defined(NQ_OS_WINDOWS)
  NQFree((void*)info->filename);
#endif
}
