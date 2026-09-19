/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_JSON_JSON_H
#define _LIBNETQ_JSON_JSON_H

#include <libnetq/string/StringPrint.h>

#if defined(JSON_BACKEND_CJSON)
# include <cjson/cJSON.h>
typedef cJSON NQJSON;
typedef cJSON NQJSON_ObjectIter;
#elif defined(JSON_BACKEND_JANSSON)
# include <jansson.h>
typedef json_t NQJSON;
typedef void NQJSON_ObjectIter;
# else
typedef void NQJSON;
typedef void NQJSON_ObjectIter;
#endif

#ifdef __cplusplus
extern "C" {
#endif

NQ_EXPORT const char* NQJSON_package(void);
NQ_EXPORT const char* NQJSON_version(void);

NQ_EXPORT NQJSON* NQJSON_fromFile(const char* filename);
NQ_EXPORT NQJSON* NQJSON_parse(const char* text);
NQ_EXPORT NQJSON* NQJSON_parse2(const char* text, size_t length);
NQ_EXPORT NQJSON* NQJSON_clone(const NQJSON*, bool deep);

NQ_EXPORT NQJSON* NQJSON_createNull(void);
NQ_EXPORT NQJSON* NQJSON_createTrue(void);
NQ_EXPORT NQJSON* NQJSON_createFalse(void);
NQ_EXPORT NQJSON* NQJSON_createBool(bool value);
NQ_EXPORT NQJSON* NQJSON_createDouble(double value);
NQ_EXPORT NQJSON* NQJSON_createInt64(int64_t value);
NQ_EXPORT NQJSON* NQJSON_createStringRef(const char* value);
NQ_EXPORT NQJSON* NQJSON_createArrayRef(void);
NQ_EXPORT NQJSON* NQJSON_createObjectRef(void);

NQ_EXPORT bool NQJSON_isNull(const NQJSON*);
NQ_EXPORT bool NQJSON_isFalse(const NQJSON*);
NQ_EXPORT bool NQJSON_isTrue(const NQJSON*);
NQ_EXPORT bool NQJSON_isBool(const NQJSON*);
NQ_EXPORT bool NQJSON_isNumber(const NQJSON*);
NQ_EXPORT bool NQJSON_isDouble(const NQJSON*);
NQ_EXPORT bool NQJSON_isInt64(const NQJSON*);
NQ_EXPORT bool NQJSON_isString(const NQJSON*);
NQ_EXPORT bool NQJSON_isArray(const NQJSON*);
NQ_EXPORT bool NQJSON_isObject(const NQJSON*);

NQ_EXPORT bool NQJSON_asBool(const NQJSON*);
NQ_EXPORT double NQJSON_asDouble(const NQJSON*);
NQ_EXPORT int64_t NQJSON_asInt64(const NQJSON*);
NQ_EXPORT const char* NQJSON_asString(const NQJSON* json);

NQ_EXPORT size_t NQJSON_stringLength(const NQJSON*);

NQ_EXPORT size_t NQJSON_arraySize(const NQJSON*);
NQ_EXPORT NQJSON* NQJSON_arrayGet(const NQJSON*, size_t index);
NQ_EXPORT bool NQJSON_arrayGetBool(const NQJSON*, size_t index, bool* value);
NQ_EXPORT bool NQJSON_arrayGetInt64(const NQJSON*, size_t index, int64_t* value);
NQ_EXPORT bool NQJSON_arrayGetDouble(const NQJSON*, size_t index, double* value);
NQ_EXPORT bool NQJSON_arrayGetString(const NQJSON*, size_t index, const char** value);
NQ_EXPORT bool NQJSON_arrayAdd(NQJSON*, NQJSON* item);

NQ_EXPORT NQJSON* NQJSON_objectGet(const NQJSON*, const char* key);
NQ_EXPORT bool NQJSON_objectGetBool(const NQJSON*, const char* name, bool* value);
NQ_EXPORT bool NQJSON_objectGetInt64(const NQJSON*, const char* name, int64_t* value);
NQ_EXPORT bool NQJSON_objectGetDouble(const NQJSON*, const char* name, double* value);
NQ_EXPORT bool NQJSON_objectGetString(const NQJSON*, const char* name, const char** value);
NQ_EXPORT bool NQJSON_objectSet(NQJSON* json, const char* key, NQJSON* item);
NQ_EXPORT bool NQJSON_objectSetBool(NQJSON* json, const char* key, bool value);
NQ_EXPORT bool NQJSON_objectSetInt64(NQJSON* json, const char* key, int64_t value);
NQ_EXPORT bool NQJSON_objectSetDouble(NQJSON* json, const char* key, double value);
NQ_EXPORT bool NQJSON_objectSetString(NQJSON* json, const char* key, const char* value);

NQ_EXPORT NQJSON_ObjectIter* NQJSON_objectIterFirst(NQJSON*);
NQ_EXPORT NQJSON_ObjectIter* NQJSON_objectIterNext(NQJSON*, NQJSON_ObjectIter* iter);
NQ_EXPORT const char* NQJSON_objectIterKey(NQJSON_ObjectIter* iter);
NQ_EXPORT NQJSON* NQJSON_objectIterValue(NQJSON_ObjectIter* iter);

NQ_EXPORT void NQJSON_release(NQJSON*);
NQ_EXPORT bool NQJSON_isEqual(NQJSON* a, NQJSON* b);

NQ_EXPORT bool NQJSON_dump(const NQJSON*, NQStringPrint* buffer);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_JSON_JSON_H */
