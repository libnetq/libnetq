/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/fs/Path.h"

#include <libnetq/UTF.h>
#include <libnetq/string/String.h>
#include <libnetq/MinMax.h>
#include <libnetq/Malloc.h>
#include <libnetq/Limits.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/CType.h>
#include <libnetq/Assert.h>

#ifdef NQ_OS_WINDOWS
#include <windows.h>
#endif

NQ_STATIC_ASSERT(sizeof(uint16_t) == sizeof(NQWChar), "Bad size of NQWChar");

static const char kPosixAbsolute[] = NQ_POSIX_PATH_SEPARATOR_STR;

static inline bool isAnyPathSeparator(char ch)
{
  return ch == NQ_POSIX_PATH_SEPARATOR || ch == NQ_WIN32_PATH_SEPARATOR;
}

struct PathWrite {
  char* start;
  char* position;
  char* end;
  char first[2];
  size_t length;
  size_t segmentCount;
};

static inline void pathWriteInit(struct PathWrite* ctx, char* buf, size_t len)
{
  ctx->start = ctx->position = buf;
  ctx->end = (buf == NULL) ? NULL : buf + len;
  ctx->length = 0;
  ctx->segmentCount = 0;
}

static inline bool isPosixAbsolute(const char* str, size_t len)
{
  return len == 1 && str[0] == NQ_POSIX_PATH_SEPARATOR;
}

static inline bool isWin32Absolute(const char* str, size_t len)
{
  return len == 2 && str[1] == ':' && NQIsAlpha(str[0]);
}

static inline void pathWritePush(struct PathWrite* ctx, const char* segment, size_t length)
{
  NQ_ASSERT(length);

  if (length == 1) {
    if (segment[0] == '.')
      return;
    if (ctx->length == 0 && NQIsPathSeparator(segment[0])) {
      if (ctx->position < ctx->end)
        *ctx->position++ = NQ_PATH_SEPARATOR;
      ctx->first[0] = NQ_PATH_SEPARATOR;
      ctx->length++;
      return;
    }
  }

  NQ_ASSERT(memchr(segment, NQ_PATH_SEPARATOR, length) == NULL);

  if (length == 2 && segment[0] == '.' && segment[1] == '.') {
    if (ctx->segmentCount != 0) {
      ctx->segmentCount--;
      if (ctx->start == NULL)
        return;
      while (ctx->start < ctx->position) {
        if (*--ctx->position == NQ_PATH_SEPARATOR) {
          if (ctx->start == ctx->position)
            ctx->position++;
          break;
        }
      }
      ctx->length = ctx->position - ctx->start;
      return;
    }
    else if (isPosixAbsolute(ctx->first, ctx->length))
      return;
    else if (isWin32Absolute(ctx->first, ctx->length))
      return;
  }
  else if (ctx->length != 0 || !(length == 2 && segment[1] == ':' && NQIsAlpha(segment[0]))) {
    ctx->segmentCount++;
  }

  if (ctx->length > 0 && !isPosixAbsolute(ctx->first, ctx->length)) {
    if (ctx->position < ctx->end)
      *ctx->position++ = NQ_PATH_SEPARATOR;
    if (ctx->length < 2)
      ctx->first[ctx->length] = NQ_PATH_SEPARATOR;
    ctx->length++;
  }

  size_t n = NQGetMin(length, ctx->end - ctx->position);
  if (n != 0) {
    memmove(ctx->position, segment, n);
    ctx->position += n;
  }

  if (ctx->length < 2) {
    ctx->first[ctx->length] = segment[0];
    if ((ctx->length + 1) < 2 && 2 <= length)
      ctx->first[ctx->length + 1] = segment[1];
  }

  ctx->length += length;
}

static inline int pathWriteFinish(struct PathWrite* ctx)
{
  if (ctx->length == 0) {
    if (ctx->position < ctx->end)
      *ctx->position++ = '.';
    ctx->length++;
  }
  else if (ctx->length == 2 && ctx->first[1] == ':' && NQIsAlpha(ctx->first[0])) {
    if (ctx->position < ctx->end)
      *ctx->position++ = NQ_PATH_SEPARATOR;
    ctx->length++;
  }

  if (ctx->position < ctx->end)
    *ctx->position = '\0';

  NQ_ASSERT(ctx->start == NULL || ctx->length == ctx->position - ctx->start);
  return (int)ctx->length;
}

struct PathRead {
  const char** paths;
  const char** curr;
  const char* pos;
  size_t count;
};

static inline void pathReadInit(struct PathRead* ctx, const char** paths, bool resolve)
{
  if (resolve) {
    for (const char** p = paths; *p; p++) {
      const char* iter = *p;
      if (isAnyPathSeparator(iter[0]) || (iter[1] == ':' && NQIsAlpha(iter[0]) && (iter[2] == '\0' || isAnyPathSeparator(iter[2]))))
        paths = p;
    }
  }

  ctx->paths = ctx->curr = paths;
  ctx->pos = NULL;
  ctx->count = 0;
}

static inline const char* pathReadNext(struct PathRead* ctx, size_t* length)
{
  while (*ctx->curr) {
    if (ctx->pos == NULL) {
      ctx->pos = *ctx->curr;
    }

    while (*ctx->pos && isAnyPathSeparator(*ctx->pos)) {
      ctx->pos++;
      if (ctx->count == 0) {
        *length = 1;
        ctx->count++;
        return kPosixAbsolute;
      }
    }

    if (*ctx->pos != '\0') {
      const char* result = ctx->pos++;
      while (*ctx->pos && !isAnyPathSeparator(*ctx->pos))
        ctx->pos++;
      *length = ctx->pos - result;
      ctx->count++;
      return result;
    }

    ctx->curr++;
    ctx->pos = NULL;
  }

  return NULL;
}

int NQPathJoin(const char** paths, char* buf, size_t len)
{
  size_t length;
  const char* segment;

  struct PathRead read;
  pathReadInit(&read, paths, false);

  struct PathWrite write;
  pathWriteInit(&write, buf, len);

  for (;;) {
    segment = pathReadNext(&read, &length);
    if (segment == NULL)
      break;
    pathWritePush(&write, segment, length);
  }

  return pathWriteFinish(&write);
}

int NQPathResolve(const char** paths, char* buf, size_t len)
{
  size_t length;
  const char* segment;

  struct PathRead read;
  pathReadInit(&read, paths, true);

  struct PathWrite write;
  pathWriteInit(&write, buf, len);

  for (;;) {
    segment = pathReadNext(&read, &length);
    if (segment == NULL)
      break;
    pathWritePush(&write, segment, length);
  }

  return pathWriteFinish(&write);
}

NQPath* NQPath_join(const char** paths)
{
  size_t length = NQPathJoin(paths, NULL, 0);
  NQPath* thiz = NQStringArray16_alloc(length);
  if (thiz == NULL)
    return NULL;
  length = NQPathJoin(paths, thiz->characters, length);
  NQStringArray16_shrink(thiz, length);
  return thiz;
}

NQPath* NQPath_resolve(const char** paths)
{
  size_t length = NQPathResolve(paths, NULL, 0);
  NQPath* thiz = NQStringArray16_alloc(length);
  if (thiz == NULL)
    return NULL;
  length = NQPathResolve(paths, thiz->characters, length);
  NQStringArray16_shrink(thiz, length);
  return thiz;
}

static inline void pathBuilderInit(NQPathBuilder* thiz)
{
  thiz->characters = thiz->buffer;
  thiz->characters[0] = '\0';
  thiz->length = 0;
  thiz->capacity = sizeof(thiz->buffer);
}

static inline void pathBuilderFinalize(NQPathBuilder* thiz)
{
  if (thiz->characters != thiz->buffer)
    NQFree(thiz->characters);
}

static bool pathBuilderReserveCapacity(NQPathBuilder* thiz, size_t newCapacity)
{
  if (NQ_UINT16_MAX < newCapacity)
    return false;

  if (newCapacity <= thiz->capacity)
    return true;

  char* oldCharacters = thiz->characters;
  char* newCharacters = (char*)NQMalloc(newCapacity);
  if (newCharacters == NULL)
    return false;

  memcpy(newCharacters, oldCharacters, thiz->length + 1);
  if (oldCharacters != thiz->buffer) {
    NQFree(oldCharacters);
  }

  thiz->characters = newCharacters;
  thiz->capacity = (uint16_t)newCapacity;

  return true;
}

static inline bool pathBuilderExpandCapacity(NQPathBuilder* thiz, size_t newMinCapacity)
{
  return pathBuilderReserveCapacity(thiz, NQGetMax(newMinCapacity, thiz->capacity + thiz->capacity / 4 + 1));
}

void NQPathBuilder_init(NQPathBuilder* thiz)
{
  pathBuilderInit(thiz);
}

void NQPathBuilder_finalize(NQPathBuilder* thiz)
{
  pathBuilderFinalize(thiz);
}

void NQPathBuilder_clear(NQPathBuilder* thiz, const char* path)
{
  pathBuilderFinalize(thiz);
  pathBuilderInit(thiz);
}

static inline bool pathBuilderJoin(NQPathBuilder* thiz, const char** paths)
{
  NQ_ASSERT(paths[0] == thiz->characters);
  size_t length = NQPathJoin(paths, NULL, 0);
  if (!pathBuilderExpandCapacity(thiz, length + 1))
    return false;
  NQ_ASSERT(thiz->characters[thiz->length] == '\0');
  if (paths[0] != thiz->characters)
    paths[0] = thiz->characters;
  thiz->length = (uint16_t)NQPathJoin(paths, thiz->characters, length + 1);
  NQ_ASSERT(thiz->characters[thiz->length] == '\0');
  return true;
}

static inline bool pathBuilderResolve(NQPathBuilder* thiz, const char** paths)
{
  NQ_ASSERT(paths[0] == thiz->characters);
  size_t length = NQPathResolve(paths, NULL, 0);
  if (!pathBuilderExpandCapacity(thiz, length + 1))
    return false;
  NQ_ASSERT(thiz->characters[thiz->length] == '\0');
  if (paths[0] != thiz->characters)
    paths[0] = thiz->characters;
  thiz->length = (uint16_t)NQPathResolve(paths, thiz->characters, length + 1);
  NQ_ASSERT(thiz->characters[thiz->length] == '\0');
  return true;
}

bool NQPathBuilder_join1(NQPathBuilder* thiz, const char* path1)
{
  const char* paths[] = { thiz->characters, path1, NULL };
  return pathBuilderJoin(thiz, paths);
}

bool NQPathBuilder_join2(NQPathBuilder* thiz, const char* path1, const char* path2)
{
  const char* paths[] = { thiz->characters, path1, path2, NULL };
  return pathBuilderJoin(thiz, paths);
}

bool NQPathBuilder_join3(NQPathBuilder* thiz, const char* path1, const char* path2, const char* path3)
{
  const char* paths[] = { thiz->characters, path1, path2, path3, NULL };
  return pathBuilderJoin(thiz, paths);
}

bool NQPathBuilder_resolve1(NQPathBuilder* thiz, const char* path1)
{
  const char* paths[] = { thiz->characters, path1, NULL };
  return pathBuilderResolve(thiz, paths);
}

bool NQPathBuilder_resolve2(NQPathBuilder* thiz, const char* path1, const char* path2)
{
  const char* paths[] = { thiz->characters, path1, path2, NULL };
  return pathBuilderResolve(thiz, paths);
}

bool NQPathBuilder_resolve3(NQPathBuilder* thiz, const char* path1, const char* path2, const char* path3)
{
  const char* paths[] = { thiz->characters, path1, path2, path3, NULL };
  return pathBuilderResolve(thiz, paths);
}

void NQPathBuilder_removeFilename(NQPathBuilder* thiz)
{
  const char* paths[] = { thiz->characters, "..", NULL };
  (void)pathBuilderJoin(thiz, paths);
}

bool NQPathBuilder_replaceFilename(NQPathBuilder* thiz, const char* filename)
{
  const char* paths[] = { thiz->characters, "..", filename, NULL };
  return pathBuilderJoin(thiz, paths);
}

void NQWinPathBuilder_init(NQWinPathBuilder* thiz)
{
  thiz->characters = thiz->buffer;
  thiz->characters[0] = L'\0';
  thiz->length = 0;
  thiz->capacity = NQ_ARRAY_LENGTH(thiz->buffer);
}

bool NQPathInfoParse(const char* path, NQPathInfo* result)
{
  return NQPathInfoParse2(path, strlen(path), result);
}

bool NQPathInfoParse2(const char* path, size_t length, NQPathInfo* result)
{
  if (length == 0) {
    result->path.characters = path;
    result->path.length = 0;
    result->dirname.characters = ".";
    result->dirname.length = 1;
    result->basename.characters = path;
    result->basename.length = 0;
    result->isAbsolute = false;
    result->isDirOnly = false;
    result->isNormalize = true;
    return true;
  }

  bool isNormalize = true;
  char lastChar = path[0];

  const char* basename = path;
  bool isAbsolute = NQIsPathSeparator(lastChar);
  int normalizeCount = (lastChar == '.') ? 1 : 3;

  for (size_t i = 1; i < length; i++) {
    char ch = path[i];
    if (ch == '\0') {
      break;
    }
    if (!NQIsPathSeparator(ch)) {
      if (NQIsPathSeparator(lastChar)) {
        basename = &path[i];
        normalizeCount = (ch == '.') ? 1 : 3;
      }
      else if (ch == '.') {
        normalizeCount++;
      }
    }
    else if (NQIsPathSeparator(lastChar) || normalizeCount < 3) {
      isNormalize = false;
    }
    lastChar = ch;
  }

  if (length == 1 && NQIsPathSeparator(lastChar)) {
    result->path.characters = path;
    result->path.length = 1;
    result->dirname.characters = path;
    result->dirname.length = 1;
    result->basename.characters = path;
    result->basename.length = 0;
    result->isAbsolute = isAbsolute;
    result->isDirOnly = true;
    result->isNormalize = isNormalize;
    return true;
  }

  result->path.characters = path;
  result->path.length = length;
  result->dirname.characters = path;
  result->dirname.length = basename - path;
  if (result->dirname.length > 1)
    result->dirname.length--;
  if (result->dirname.length == 0) {
    result->dirname.characters = ".";
    result->dirname.length = 1;
  }
  result->basename.characters = basename;
  result->basename.length = length - (basename - path);
  result->isAbsolute = isAbsolute;
  result->isDirOnly = NQIsPathSeparator(lastChar);
  if (result->isDirOnly)
    result->basename.length--;
  result->isNormalize = isNormalize;

  return true;
}

size_t NQPathFrom(char* buffer, size_t n, const NQWChar* path)
{
  if (n == 0)
    return 0;

  NQUChar character;
  const uint16_t* utf16Start = (const uint16_t*)path;
  uint8_t* utf8Start = (uint8_t*)buffer;
  uint8_t* utf8End = utf8Start + n;

  while (*utf16Start != '\0') {
    character = NQUCharNext16(utf16Start, utf16Start + 2, &utf16Start);
    if (!NQIsUChar(character)) {
      NQ_ASSERT(0);
      *buffer = '\0';
      return 0; // TODO: return length
    }

    if (character == NQ_WIN32_PATH_SEPARATOR)
      character = NQ_PATH_SEPARATOR;

    if (!NQUCharPush8(utf8Start, utf8End, &utf8Start, character)) {
      NQ_ASSERT(0);
      *buffer = '\0';
      return 0; // TODO: return length
    }
  }
  
  if (utf8Start < utf8End)
    *utf8Start = '\0';

  return utf8Start - (uint8_t*)buffer;
}

size_t NQGetAbsolutePath(char* buffer, size_t n, const char* path)
{
  // TODO
  // https://chromium.googlesource.com/native_client/nacl-newlib/+/a9ae3c60b36dea3d8a10e18b1b6db952d21268c2/newlib/libc/sys/linux/realpath.c
  //realpath(const char *path, char *resolved_path);
  //  NQIsAbsolutePath(path)
  size_t len = NQStrlen(path);
  size_t sz = NQGetMin(n, len + 1);
  memcpy(buffer, path, sz);
  return len;
}

size_t NQWinPathFrom(NQWChar* buffer, size_t n, const char* path)
{
  if (n == 0)
    return 0;

  NQUChar character;
  const uint8_t* utf8Start = (const uint8_t*)path;
  uint16_t* utf16Start = (uint16_t*)buffer;
  uint16_t* utf16End = utf16Start + n;

  while (*utf8Start != '\0') {
    character = NQUCharNext8(utf8Start, utf8Start + 6, &utf8Start);
    if (!NQIsUChar(character)) {
      NQ_ASSERT(0);
      *buffer = '\0';
      return 0; // TODO: return length
    }

    if (NQIsPathSeparator(character))
      character = NQ_WIN32_PATH_SEPARATOR;

    if (!NQUCharPush16(utf16Start, utf16End, &utf16Start, character)) {
      NQ_ASSERT(0);
      *buffer = '\0';
      return 0; // TODO: return length
    }
  }
  
  if (utf16Start < utf16End)
    *utf16Start = '\0';

  return utf16Start - (uint16_t*)buffer;
}

size_t NQGetAbsoluteWinPath(NQWChar* buffer, size_t n, const char* path)
{
#ifdef NQ_OS_WINDOWS
  WCHAR winpath[MAX_PATH];
  size_t length = NQWinPathFrom(winpath, sizeof(winpath), path);
  if (length != 0 && length < NQ_ARRAY_LENGTH(winpath) && n <= MAXDWORD)
    return (size_t)GetFullPathNameW(winpath, (DWORD)n, buffer, NULL);
#endif

  return 0;
}

bool NQIsAbsolutePath(const char* path) {
  if (NQIsAbsolutePosixPath(path))
    return true;
  if (path[0] == '\0')
    return false;
  if (path[1] == ':' && NQIsAlpha(path[0]) && isAnyPathSeparator(path[2]))
    return true;
  return false;
}

const char* NQGetFilename(const char* path)
{
  const char* filename = path;
  const char* p = path;
  while (*p) {
    if (isAnyPathSeparator(*p++)) {
      filename = p;
    }
  }
  return filename;
}

const char* NQGetExtname(const char* path)
{
  return NQStrrchr(path, '.');
}

bool NQPathStartsWith(const char* path, const char* search)
{
  char lastChar = '\0';
  while (*search) {
    lastChar = *path;
    if (lastChar == '\0')
      return false;
    if (lastChar != *search && !(isAnyPathSeparator(lastChar) && isAnyPathSeparator(*search)))
      return false;
    path++;
    search++;
  }

  if (*path == '\0' || isAnyPathSeparator(*path))
    return true;

  return isAnyPathSeparator(lastChar);
}
