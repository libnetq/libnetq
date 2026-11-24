/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_PRIMITIVESTORAGE_H
#define _LIBNETQ_PRIMITIVESTORAGE_H

#include <libnetq/List.h>
#include <libnetq/PrimitiveType.h>
#include <libnetq/JSON.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQPrimitiveStorage NQPrimitiveStorage;

struct NQPrimitiveStorage {
  NQPrimitiveStorage* parent;
  NQListHead entryList;
};

NQ_EXPORT void NQPrimitiveStorage_init(NQPrimitiveStorage*, NQPrimitiveStorage* parent);
NQ_EXPORT void NQPrimitiveStorage_finalize(NQPrimitiveStorage*);

NQ_EXPORT bool NQPrimitiveStorage_getBool(NQPrimitiveStorage*, const char* name, bool* value);
NQ_EXPORT bool NQPrimitiveStorage_setBool(NQPrimitiveStorage*, const char* name, bool value);

NQ_EXPORT bool NQPrimitiveStorage_getUint32(NQPrimitiveStorage*, const char* name, uint32_t* value);
NQ_EXPORT bool NQPrimitiveStorage_setUint32(NQPrimitiveStorage*, const char* name, uint32_t value);

NQ_EXPORT bool NQPrimitiveStorage_getString(NQPrimitiveStorage*, const char* name, const char** value);
NQ_EXPORT bool NQPrimitiveStorage_setString(NQPrimitiveStorage*, const char* name, const char* value);

NQ_EXPORT bool NQPrimitiveStorage_mergeJSON(NQPrimitiveStorage* storage, NQJSON* json);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_PRIMITIVESTORAGE_H */
