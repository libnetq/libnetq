/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_PATH_H
#define _LIBNETQ_FS_PATH_H

#include <libnetq/Basic.h>
#include <libnetq/string/StringArray.h>
#include <libnetq/string/StringRange.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_POSIX_PATH_DELIMITER     ':'
#define NQ_WIN32_PATH_DELIMITER     ';'
#define NQ_POSIX_PATH_DELIMITER_STR ":"
#define NQ_WIN32_PATH_DELIMITER_STR ";"

#define NQ_POSIX_PATH_SEPARATOR     '/'
#define NQ_WIN32_PATH_SEPARATOR     '\\'
#define NQ_POSIX_PATH_SEPARATOR_STR "/"
#define NQ_WIN32_PATH_SEPARATOR_STR "\\"

#define NQ_PATH_SEPARATOR     NQ_POSIX_PATH_SEPARATOR
#define NQ_PATH_SEPARATOR_STR NQ_POSIX_PATH_SEPARATOR_STR

#define NQIsPathSeparator(c) ((c) == NQ_PATH_SEPARATOR)

#define NQ_ROOT_DIR_STR "/"
#define NQ_PATH_CURRENT_DIR_STR "."
#define NQ_PATH_PARENT_DIR_STR  ".."

NQ_EXPORT int NQPathJoin(const char** paths, char* buf, size_t len);
NQ_EXPORT int NQPathResolve(const char** paths, char* buf, size_t len);

static inline int NQPathJoin1(const char* path1, char* buf, size_t len)
{
  const char* paths[] = { path1, NULL };
  return NQPathJoin(paths, buf, len);
}

static inline int NQPathResolve1(const char* path1, char* buf, size_t len)
{
  const char* paths[] = { path1, NULL };
  return NQPathResolve(paths, buf, len);
}

static inline int NQPathJoin2(const char* path1, const char* path2, char* buf, size_t len)
{
  const char* paths[] = { path1, path2, NULL };
  return NQPathJoin(paths, buf, len);
}

static inline int NQPathResolve2(const char* path1, const char* path2, char* buf, size_t len)
{
  const char* paths[] = { path1, path2, NULL };
  return NQPathJoin(paths, buf, len);
}

static inline int NQPathJoin3(const char* path1, const char* path2, const char* path3, char* buf, size_t len)
{
  const char* paths[] = { path1, path2, path3, NULL };
  return NQPathJoin(paths, buf, len);
}

static inline int NQPathResolve3(const char* path1, const char* path2, const char* path3, char* buf, size_t len)
{
  const char* paths[] = { path1, path2, path3, NULL };
  return NQPathJoin(paths, buf, len);
}

typedef struct NQStringArray16 NQPath;

NQ_EXPORT NQPath* NQPath_join(const char** paths);
NQ_EXPORT NQPath* NQPath_resolve(const char** paths);

static inline NQPath* NQPath_join1(const char* path1)
{
  const char* paths[] = { path1, NULL };
  return NQPath_join(paths);
}

static inline NQPath* NQPath_resolve1(const char* path1)
{
  const char* paths[] = { path1, NULL };
  return NQPath_resolve(paths);
}

static inline NQPath* NQPath_join2(const char* path1, const char* path2)
{
  const char* paths[] = { path1, path2, NULL };
  return NQPath_join(paths);
}

static inline NQPath* NQPath_resolve2(const char* path1, const char* path2)
{
  const char* paths[] = { path1, path2, NULL };
  return NQPath_resolve(paths);
}

static inline NQPath* NQPath_join3(const char* path1, const char* path2, const char* path3)
{
  const char* paths[] = { path1, path2, path3, NULL };
  return NQPath_join(paths);
}

static inline NQPath* NQPath_resolve3(const char* path1, const char* path2, const char* path3)
{
  const char* paths[] = { path1, path2, path3, NULL };
  return NQPath_resolve(paths);
}

static inline void NQPath_destroy(NQPath* thiz)
{
  NQStringArray16_destroy(thiz);
}

#define NQPath_characters(thiz) NQStringArray16_characters(thiz)

static inline size_t NQPath_length(const NQPath* thiz)
{
  return NQStringArray16_length(thiz);
}

typedef struct NQPathBuilder NQPathBuilder;
struct NQPathBuilder {
  char* characters;
  uint16_t length;
  uint16_t capacity;
  char buffer[80];
};

NQ_EXPORT void NQPathBuilder_init(NQPathBuilder*);
NQ_EXPORT void NQPathBuilder_finalize(NQPathBuilder*);

#define NQPathBuilder_characters(thiz) (thiz)->characters
static inline size_t NQPathBuilder_length(NQPathBuilder* thiz)
{
  return thiz->length;
}

NQ_EXPORT void NQPathBuilder_clear(NQPathBuilder*, const char* path);
NQ_EXPORT bool NQPathBuilder_join1(NQPathBuilder*, const char* path1);
NQ_EXPORT bool NQPathBuilder_join2(NQPathBuilder*, const char* path1, const char* path2);
NQ_EXPORT bool NQPathBuilder_join3(NQPathBuilder*, const char* path1, const char* path2, const char* path3);
NQ_EXPORT bool NQPathBuilder_resolve1(NQPathBuilder*, const char* path1);
NQ_EXPORT bool NQPathBuilder_resolve2(NQPathBuilder*, const char* path1, const char* path2);
NQ_EXPORT bool NQPathBuilder_resolve3(NQPathBuilder*, const char* path1, const char* path2, const char* path3);
NQ_EXPORT void NQPathBuilder_removeFilename(NQPathBuilder*);
NQ_EXPORT bool NQPathBuilder_replaceFilename(NQPathBuilder*, const char* filename);

typedef struct NQWinPathBuilder NQWinPathBuilder;
struct NQWinPathBuilder {
  NQWChar* characters;
  uint16_t length;
  uint16_t capacity;
  NQWChar buffer[80];
};

NQ_EXPORT void NQWinPathBuilder_init(NQWinPathBuilder*);

typedef struct NQPathInfo NQPathInfo;
struct NQPathInfo {
  NQStringRange path;
  NQStringRange dirname;
  NQStringRange basename;
  bool isAbsolute;
  bool isDirOnly;
  bool isNormalize;
};

NQ_EXPORT bool NQPathInfoParse(const char* path, NQPathInfo* result);
NQ_EXPORT bool NQPathInfoParse2(const char* path, size_t length, NQPathInfo* result);

NQ_EXPORT size_t NQPathFrom(char* buffer, size_t n, const NQWChar* path);
NQ_EXPORT size_t NQGetAbsolutePath(char* buffer, size_t n, const char* path);
NQ_EXPORT size_t NQWinPathFrom(NQWChar* buffer, size_t n, const char* path);
NQ_EXPORT size_t NQGetAbsoluteWinPath(NQWChar* buffer, size_t n, const char* path);

static inline bool NQIsAbsolutePosixPath(const char* path)
{
  return path[0] == NQ_PATH_SEPARATOR;
}

static inline bool NQIsRootPosixPath(const char* path)
{
  return path[0] == NQ_PATH_SEPARATOR && path[1] == '\0';
}

NQ_EXPORT bool NQIsAbsolutePath(const char* path);
NQ_EXPORT const char* NQGetFilename(const char* path);
NQ_EXPORT const char* NQGetExtname(const char* path);

NQ_EXPORT bool NQPathStartsWith(const char* path, const char* search);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_FS_PATH_H */
