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

#ifdef NQCONFIG_USE_CJSON_JSON

#include <libnetq/string/String.h>
#include <libnetq/string/StringRef.h>
#include <libnetq/Math.h>
#include <libnetq/Limits.h>

const char* NQJSON_package(void)
{
  return "cJSON";
}

const char* NQJSON_version(void)
{
  return cJSON_Version();
}

NQJSON* NQJSON_fromFile(const char* filename)
{
  NQString* text = NQString_fromFile(filename);
  if (text == NULL)
    return NULL;
  NQJSON* json = cJSON_ParseWithLength(NQString_characters(text), NQString_length(text));
  NQString_release(text);
  return json;
}

NQJSON* NQJSON_parse(const char* text)
{
  return cJSON_Parse(text);
}

NQJSON* NQJSON_parse2(const char* text, size_t length)
{
  return cJSON_ParseWithLength(text, length);
}

NQJSON* NQJSON_clone(const NQJSON* json, bool deep)
{
  return cJSON_Duplicate(json, deep); // FIXME
}

NQJSON* NQJSON_createNull(void)
{
  return cJSON_CreateNull();
}

NQJSON* NQJSON_createTrue(void)
{
  return cJSON_CreateTrue();
}

NQJSON* NQJSON_createFalse(void)
{
  return cJSON_CreateFalse();
}

NQJSON* NQJSON_createBool(bool value)
{
  return cJSON_CreateBool(value);
}

NQJSON* NQJSON_createDouble(double value)
{
  return cJSON_CreateNumber(value);
}

NQJSON* NQJSON_createInt64(int64_t value)
{
  return cJSON_CreateNumber((double)value);
}

NQJSON* NQJSON_createStringRef(const char* value)
{
  return cJSON_CreateStringReference(value);
}

NQJSON* NQJSON_createArrayRef(void)
{
  return cJSON_CreateArrayReference(NULL);
}

NQJSON* NQJSON_createObjectRef(void)
{
  return cJSON_CreateObjectReference(NULL);
}

bool NQJSON_isNull(const NQJSON* json)
{
  return cJSON_IsNull((const cJSON*)json);
}

bool NQJSON_isFalse(const NQJSON* json)
{
  return cJSON_IsFalse((const cJSON*)json);
}

bool NQJSON_isTrue(const NQJSON* json)
{
  return cJSON_IsTrue((const cJSON*)json);
}

bool NQJSON_isBool(const NQJSON* json)
{
  return cJSON_IsBool((const cJSON*)json);
}

bool NQJSON_isNumber(const NQJSON* json)
{
  return cJSON_IsNumber((const cJSON*)json);
}

bool NQJSON_isDouble(const NQJSON* json)
{
  return cJSON_IsNumber((const cJSON*)json);
}

bool NQJSON_isInt64(const NQJSON* json)
{
  double num = cJSON_GetNumberValue(json);
  if (!isfinite(num) || num != floor(num)) {
    return false;
  }
  /* (double)NQ_INT64_MAX rounds up to 2^63 because INT64_MAX (2^63 - 1)
   * isn't exactly representable as a double, so this comparison must be
   * '>=' (not '>') to correctly reject num == 2^63, which is already
   * out of range for int64_t. */
  if (num < (double)NQ_INT64_MIN || num >= (double)NQ_INT64_MAX) {
    return false;
  }
  return true;
}

bool NQJSON_isString(const NQJSON* json)
{
  return cJSON_IsString((const cJSON*)json);
}

bool NQJSON_isArray(const NQJSON* json)
{
  return cJSON_IsArray((const cJSON*)json);
}

bool NQJSON_isObject(const NQJSON* json)
{
  return cJSON_IsObject((const cJSON*)json);
}

bool NQJSON_asBool(const NQJSON* json)
{
  return cJSON_IsTrue((const cJSON*)json);
}

double NQJSON_asDouble(const NQJSON* json)
{
  return cJSON_GetNumberValue((const cJSON*)json);
}

int64_t NQJSON_asInt64(const NQJSON* json)
{
  return (int64_t)cJSON_GetNumberValue((const cJSON*)json);
}

const char* NQJSON_asString(const NQJSON* json)
{
  return cJSON_GetStringValue((const cJSON*)json);
}

size_t NQJSON_stringLength(const NQJSON* json)
{
  const char* str = cJSON_GetStringValue((const cJSON*)json);
  return str ? NQStrlen(str) : 0;
}

size_t NQJSON_arraySize(const NQJSON* json)
{
  return (size_t)cJSON_GetArraySize((const cJSON*)json);
}

NQJSON* NQJSON_arrayGet(const NQJSON* json, size_t index)
{
  return cJSON_GetArrayItem(json, (int)index);
}

bool NQJSON_arrayAdd(NQJSON* json, NQJSON* item)
{
  return cJSON_AddItemReferenceToArray(json, item);
}

NQJSON* NQJSON_objectGet(const NQJSON* json, const char* key)
{
  return cJSON_GetObjectItemCaseSensitive(json, key);
}

bool NQJSON_objectSet(NQJSON* json, const char* key, NQJSON* item)
{
  return cJSON_AddItemReferenceToObject(json, key, item) ? true : false;
}

bool NQJSON_objectSetBool(NQJSON* json, const char* key, bool value)
{
  return cJSON_AddBoolToObject(json, key, value) != NULL;
}

bool NQJSON_objectSetInt64(NQJSON* json, const char* key, int64_t value)
{
  return cJSON_AddNumberToObject(json, key, (double)value) != NULL;
}

bool NQJSON_objectSetDouble(NQJSON* json, const char* key, double value)
{
  return cJSON_AddNumberToObject(json, key, value) != NULL;
}

bool NQJSON_objectSetString(NQJSON* json, const char* key, const char* value)
{
  return cJSON_AddStringToObject(json, key, value) != NULL;
}

NQJSON_ObjectIter* NQJSON_objectIterFirst(NQJSON* json)
{
  return json->child;
}

NQJSON_ObjectIter* NQJSON_objectIterNext(NQJSON* json, NQJSON_ObjectIter* iter)
{
  NQ_UNUSED_PARAM(json);
  return iter->next;
}

const char* NQJSON_objectIterKey(NQJSON_ObjectIter* iter)
{
  return iter->string;
}

NQJSON* NQJSON_objectIterValue(NQJSON_ObjectIter* iter)
{
  return iter;
}

void NQJSON_release(NQJSON* json)
{
  cJSON_Delete(json);
}

bool NQJSON_isEqual(NQJSON* a, NQJSON* b)
{
  return cJSON_Compare(a, b, true);
}
#endif
