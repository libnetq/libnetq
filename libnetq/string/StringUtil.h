/*
 * MIT License
 *
 * Copyright (c) 2022-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_STRING_STRINGUTIL_H
#define _LIBNETQ_STRING_STRINGUTIL_H

#include <libnetq/VA.h>
#include <libnetq/string/String.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_NIL '\0'

#define NQ_CSTR_LENGTH(s) (NQ_ARRAY_LENGTH(s)-1)

NQ_EXPORT const char* NQCStrEmpty(void);
NQ_EXPORT const char* NQCStrEnsure(const char* str);
NQ_EXPORT void NQCStrFree(const char* str);
NQ_EXPORT size_t NQCStrLen16(const uint16_t* str);
NQ_EXPORT char* NQCStrDuplicate(const char* str);
NQ_EXPORT char* NQCStrDuplicateWithLength(const char* str, size_t len);
NQ_EXPORT const char* NQCStrFindCStrn(const char* s1, const char* s2, size_t n);
NQ_EXPORT bool NQCStrStartsWith(const char* str, const char* search);

NQ_EXPORT char* NQCStrFormat(const char* format, ...) NQ_ATTRIBUTE_PRINTF(1, 2);
NQ_EXPORT char* NQCStrFormatV(const char* format, va_list args);

static inline bool NQCStrIsEmpty(const char* str)
{
  return *str == NQ_NIL;
}

static inline bool NQCStrIsNullOrEmpty(const char* str)
{
  return str == NULL || NQCStrIsEmpty(str);
}

static inline const char* NQCStrEmptyIfNull(const char* str)
{
  return str == NULL ? NQCStrEmpty() : str;
}

static inline bool NQCStrIsEqual(const char* str1, const char* str2, size_t len2)
{
  return NQStrncmp(str2, str1, len2) == 0 && str1[len2] == '\0';
}

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_STRING_STRINGUTIL_H */
