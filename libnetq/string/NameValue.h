/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_STRING_NAMEVALUE_H
#define _LIBNETQ_STRING_NAMEVALUE_H

#include <libnetq/List.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQNameValue NQNameValue;
struct NQNameValue {
  NQListHead list;
};

typedef struct NQNameValueEntry NQNameValueEntry;
struct NQNameValueEntry {
  NQListHead list;
  char* name;
  char value[1];
};

static inline void NQNameValue_init(NQNameValue* thiz)
{
  NQListHead_init(&thiz->list);
}

NQ_EXPORT void NQNameValue_finalize(NQNameValue*);
NQ_EXPORT bool NQNameValue_add(NQNameValue*, const char* name, const char* value);

NQ_EXPORT NQNameValueEntry* NQNameValue_first(NQNameValue*);
NQ_EXPORT NQNameValueEntry* NQNameValue_next(NQNameValue*, NQNameValueEntry* entry);

static inline const char* NQNameValueEntryName(NQNameValueEntry* entry)
{
  return entry->name;
}

static inline const char* NQNameValueEntryValue(NQNameValueEntry* entry)
{
  return entry->value;
}

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_STRING_NAMEVALUE_H */
