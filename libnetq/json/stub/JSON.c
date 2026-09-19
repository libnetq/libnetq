/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_CLASS_NAME "NQJSON"
#define NQ_LOG_TAG NQ_CLASS_NAME

#include "config.h"
#include "libnetq/json/JSON.h"

#ifdef JSON_BACKEND_STUB

const char* NQJSON_package(void)
{
  return NULL;
}

const char* NQJSON_version(void)
{
  return NULL;
}

NQJSON* NQJSON_fromFile(const char* filename)
{
  NQ_UNUSED_PARAM(filename);
  return NULL;
}

NQJSON* NQJSON_parse(const char* text)
{
  NQ_UNUSED_PARAM(text);
  return NULL;
}

NQJSON* NQJSON_parse2(const char* text, size_t length)
{
  NQ_UNUSED_PARAM(text);
  NQ_UNUSED_PARAM(length);
  return NULL;
}

NQJSON* NQJSON_clone(const NQJSON* json, bool deep)
{
  NQ_UNUSED_PARAM(json);
  return NULL;
}

NQJSON* NQJSON_createNull(void)
{
  return NULL;
}

NQJSON* NQJSON_createTrue(void)
{
  return NULL;
}

NQJSON* NQJSON_createFalse(void)
{
  return NULL;
}

NQJSON* NQJSON_createBool(bool value)
{
  NQ_UNUSED_PARAM(value);
  return NULL;
}

NQJSON* NQJSON_createDouble(double value)
{
  NQ_UNUSED_PARAM(value);
  return NULL;
}

NQJSON* NQJSON_createInt64(int64_t value)
{
  NQ_UNUSED_PARAM(value);
  return NULL;
}

NQJSON* NQJSON_createStringRef(const char* value)
{
  NQ_UNUSED_PARAM(value);
  return NULL;
}

NQJSON* NQJSON_createArrayRef(void)
{
  return NULL;
}

NQJSON* NQJSON_createObjectRef(void)
{
  return NULL;
}

bool NQJSON_isNull(const NQJSON* json)
{
  NQ_UNUSED_PARAM(json);
  return false;
}

bool NQJSON_isFalse(const NQJSON* json)
{
  NQ_UNUSED_PARAM(json);
  return false;
}

bool NQJSON_isTrue(const NQJSON* json)
{
  NQ_UNUSED_PARAM(json);
  return false;
}

bool NQJSON_isBool(const NQJSON* json)
{
  NQ_UNUSED_PARAM(json);
  return false;
}

bool NQJSON_isNumber(const NQJSON* json)
{
  NQ_UNUSED_PARAM(json);
  return false;
}

bool NQJSON_isDouble(const NQJSON* json)
{
  NQ_UNUSED_PARAM(json);
  return false;
}

bool NQJSON_isInt64(const NQJSON* json)
{
  NQ_UNUSED_PARAM(json);
  return false;
}

bool NQJSON_isString(const NQJSON* json)
{
  NQ_UNUSED_PARAM(json);
  return false;
}

bool NQJSON_isArray(const NQJSON* json)
{
  NQ_UNUSED_PARAM(json);
  return false;
}

bool NQJSON_isObject(const NQJSON* json)
{
  NQ_UNUSED_PARAM(json);
  return false;
}

bool NQJSON_asBool(const NQJSON* json)
{
  NQ_UNUSED_PARAM(json);
  return false;
}

#ifdef NQ_USE_FLOATING_POINT
double NQJSON_asDouble(const NQJSON* json)
{
  NQ_UNUSED_PARAM(json);
  return 0;
}
#endif

int64_t NQJSON_asInt64(const NQJSON* json)
{
  NQ_UNUSED_PARAM(json);
  return 0;
}

const char* NQJSON_asString(const NQJSON* json)
{
  NQ_UNUSED_PARAM(json);
  return NULL;
}

size_t NQJSON_stringLength(const NQJSON* json)
{
  NQ_UNUSED_PARAM(json);
  return 0;
}

size_t NQJSON_arraySize(const NQJSON* json)
{
  NQ_UNUSED_PARAM(json);
  return 0;
}

NQJSON* NQJSON_arrayGet(const NQJSON* json, size_t index)
{
  NQ_UNUSED_PARAM(json);
  NQ_UNUSED_PARAM(index);
  return NULL;
}

bool NQJSON_arrayAdd(NQJSON* json, NQJSON* item)
{
  NQ_UNUSED_PARAM(json);
  NQ_UNUSED_PARAM(item);
  return false;
}

NQJSON* NQJSON_objectGet(const NQJSON* json, const char* key)
{
  NQ_UNUSED_PARAM(json);
  NQ_UNUSED_PARAM(key);
  return NULL;
}

bool NQJSON_objectSet(NQJSON* json, const char* key, NQJSON* item)
{
  NQ_UNUSED_PARAM(json);
  NQ_UNUSED_PARAM(key);
  NQ_UNUSED_PARAM(item);
  return false;
}

bool NQJSON_objectSetBool(NQJSON* json, const char* key, bool value)
{
  NQ_UNUSED_PARAM(json);
  NQ_UNUSED_PARAM(key);
  NQ_UNUSED_PARAM(value);
  return false;
}

bool NQJSON_objectSetInt64(NQJSON* json, const char* key, int64_t value)
{
  NQ_UNUSED_PARAM(json);
  NQ_UNUSED_PARAM(key);
  NQ_UNUSED_PARAM(value);
  return false;
}

bool NQJSON_objectSetDouble(NQJSON* json, const char* key, double value)
{
  NQ_UNUSED_PARAM(json);
  NQ_UNUSED_PARAM(key);
  NQ_UNUSED_PARAM(value);
  return false;
}

bool NQJSON_objectSetString(NQJSON* json, const char* key, const char* value)
{
  NQ_UNUSED_PARAM(json);
  NQ_UNUSED_PARAM(key);
  NQ_UNUSED_PARAM(value);
  return false;
}

NQJSON_ObjectIter* NQJSON_objectIterFirst(NQJSON* json)
{
  NQ_UNUSED_PARAM(json);
  return NULL;
}

NQJSON_ObjectIter* NQJSON_objectIterNext(NQJSON* json, NQJSON_ObjectIter* iter)
{
  NQ_UNUSED_PARAM(json);
  NQ_UNUSED_PARAM(iter);
  return NULL;
}

const char* NQJSON_objectIterKey(NQJSON_ObjectIter* iter)
{
  NQ_UNUSED_PARAM(iter);
  return NULL;
}

NQJSON* NQJSON_objectIterValue(NQJSON_ObjectIter* iter)
{
  NQ_UNUSED_PARAM(iter);
  return NULL;
}

void NQJSON_release(NQJSON* json)
{
  NQ_UNUSED_PARAM(json);
}

bool NQJSON_isEqual(NQJSON* a, NQJSON* b)
{
  NQ_UNUSED_PARAM(a);
  NQ_UNUSED_PARAM(b);
  return false;
}

#endif
