/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_STRING_STRINGDATA_H
#define _LIBNETQ_STRING_STRINGDATA_H

#include <libnetq/string/StringRange.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQStringRange NQStringData;

NQ_EXPORT void NQStringData_init(NQStringData*);
NQ_EXPORT void NQStringData_finalize(NQStringData*);

NQ_EXPORT bool NQStringData_alloc(NQStringData*, size_t length);

NQ_EXPORT bool NQStringData_set(NQStringData*, const char* characters);
NQ_EXPORT bool NQStringData_set2(NQStringData*, const char* characters, size_t length);

NQ_EXPORT bool NQStringData_printf(NQStringData*, const char* format, ...) NQ_ATTRIBUTE_PRINTF(2, 3);
NQ_EXPORT bool NQStringData_vprintf(NQStringData*, const char* format, va_list args);

NQ_EXPORT bool NQStringData_setw(NQStringData*, const NQWChar* characters);
NQ_EXPORT bool NQStringData_setw2(NQStringData*, const NQWChar* characters, size_t length);

#define NQStringData_characters(thiz) (thiz)->characters

static inline size_t NQStringData_length(const NQStringData* thiz)
{
  return thiz->length;
}

static inline bool NQStringData_isEmpty(const NQStringData* thiz)
{
  return thiz->length == 0;
}

static inline bool NQStringData_isEqual(const NQStringData* thiz, const char* str)
{
  return NQStringRangeIsEqual(thiz, str);
}

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_STRING_STRINGDATA_H */
