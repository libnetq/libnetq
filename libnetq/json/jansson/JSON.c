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

#ifdef JSON_BACKEND_JANSSON

#include <libnetq/Log.h>

const char* NQJSON_package(void)
{
  return "jansson";
}

const char* NQJSON_version(void)
{
  return JANSSON_VERSION;
}

NQJSON* NQJSON_fromFile(const char* filename)
{
  json_error_t error;
  NQJSON* thiz = json_load_file(filename, 0, &error);
  if (thiz == NULL) {
    NQ_LOGE("%s", error.text);
  }
  return thiz;
}

NQJSON* NQJSON_parse(const char* text)
{
  json_error_t error;
  NQJSON* thiz = json_loads(text, JSON_DECODE_ANY, &error);
  if (thiz == NULL) {
    NQ_LOGE("%s", error.text);
  }
  return thiz;
}

NQJSON* NQJSON_parse2(const char* text, size_t length)
{
  json_error_t error;
  NQJSON* thiz = json_loadb(text, length, JSON_DECODE_ANY, &error);
  if (thiz == NULL) {
    NQ_LOGE("%s", error.text);
  }
  return thiz;
}

NQJSON* NQJSON_clone(const NQJSON* json, bool deep)
{
  return deep ? json_deep_copy(json) : json_copy((json_t*)json);
}

NQJSON* NQJSON_createNull(void)
{
  return json_null();
}

NQJSON* NQJSON_createTrue(void)
{
  return json_true();
}

NQJSON* NQJSON_createFalse(void)
{
  return json_false();
}

NQJSON* NQJSON_createBool(bool value)
{
  return json_boolean(value);
}

NQJSON* NQJSON_createDouble(double value)
{
  return json_real(value);
}

NQJSON* NQJSON_createInt64(int64_t value)
{
  return json_integer(value);
}

NQJSON* NQJSON_createStringRef(const char* value)
{
  return json_string(value);
}

NQJSON* NQJSON_createArrayRef(void)
{
  return json_array();
}

NQJSON* NQJSON_createObjectRef(void)
{
  return json_object();
}

bool NQJSON_isNull(const NQJSON* json)
{
  return json_is_null(json);
}

bool NQJSON_isFalse(const NQJSON* json)
{
  return json_is_false(json);
}

bool NQJSON_isTrue(const NQJSON* json)
{
  return json_is_true(json);
}

bool NQJSON_isBool(const NQJSON* json)
{
  return json_is_boolean(json);
}

bool NQJSON_isNumber(const NQJSON* json)
{
  return json_is_number(json);
}

bool NQJSON_isDouble(const NQJSON* json)
{
  return json_is_real(json);
}

bool NQJSON_isInt64(const NQJSON* json)
{
  return json_is_integer(json);
}

bool NQJSON_isString(const NQJSON* json)
{
  return json_is_string(json);
}

bool NQJSON_isArray(const NQJSON* json)
{
  return json_is_array(json);
}

bool NQJSON_isObject(const NQJSON* json)
{
  return json_is_object(json);
}

bool NQJSON_asBool(const NQJSON* json)
{
  return json_boolean_value(json);
}

double NQJSON_asDouble(const NQJSON* json)
{
  return json_is_real(json) ? json_real_value(json) : (double)json_integer_value(json);
}

int64_t NQJSON_asInt64(const NQJSON* json)
{
  return json_is_integer(json) ? json_integer_value(json) : (int64_t)json_real_value(json);
}

const char* NQJSON_asString(const NQJSON* json)
{
  return json_string_value(json);
}

size_t NQJSON_stringLength(const NQJSON* json)
{
  return json_string_length(json);
}

size_t NQJSON_arraySize(const NQJSON* json)
{
  return json_array_size(json);
}

NQJSON* NQJSON_arrayGet(const NQJSON* json, size_t index)
{
  return json_array_get(json, index);
}

bool NQJSON_arrayAdd(NQJSON* json, NQJSON* item)
{
  return json_array_append(json, item) == 0;
}

NQJSON* NQJSON_objectGet(const NQJSON* json, const char* key)
{
  return json_object_get(json, key);
}

bool NQJSON_objectSet(NQJSON* json, const char* key, NQJSON* item)
{
  return json_object_set(json, key, item) == 0;
}

bool NQJSON_objectSetBool(NQJSON* json, const char* key, bool value)
{
  return json_object_set_new(json, key, json_boolean(value)) == 0;
}

bool NQJSON_objectSetInt64(NQJSON* json, const char* key, int64_t value)
{
  return json_object_set_new(json, key, json_integer(value)) == 0;
}

bool NQJSON_objectSetDouble(NQJSON* json, const char* key, double value)
{
  return json_object_set_new(json, key, json_real(value)) == 0;
}

bool NQJSON_objectSetString(NQJSON* json, const char* key, const char* value)
{
  return json_object_set_new(json, key, json_string(value)) == 0;
}

NQJSON_ObjectIter* NQJSON_objectIterFirst(NQJSON* json)
{
  return json_object_iter(json);
}

NQJSON_ObjectIter* NQJSON_objectIterNext(NQJSON* json, NQJSON_ObjectIter* iter)
{
  return json_object_iter_next(json, iter);
}

const char* NQJSON_objectIterKey(NQJSON_ObjectIter* iter)
{
  return json_object_iter_key(iter);
}

NQJSON* NQJSON_objectIterValue(NQJSON_ObjectIter* iter)
{
  return json_object_iter_value(iter);
}

void NQJSON_release(NQJSON* json)
{
  json_decref(json);
}

bool NQJSON_isEqual(NQJSON* a, NQJSON* b)
{
  return json_equal(a, b) == 1;
}
#endif
