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

#include <libnetq/string/StringPrint.h>
#include <libnetq/json/JSONWriter.h>

static inline bool jsonToBool(const NQJSON* json, bool* value)
{
  if (!NQJSON_isBool(json))
    return false;
  if (value)
    *value = NQJSON_asBool(json);
  return true;
}

static inline bool jsonToInt64(const NQJSON* json, int64_t* value)
{
  if (!NQJSON_isInt64(json))
    return false;
  if (value)
    *value = NQJSON_asInt64(json);
  return true;
}

#ifdef NQ_USE_FLOATING_POINT
static inline bool jsonToDouble(const NQJSON* json, double* value)
{
  if (!NQJSON_isDouble(json))
    return false;
  if (value)
    *value = NQJSON_asDouble(json);
  return true;
}
#endif

static inline bool jsonToString(const NQJSON* json, const char** value)
{
  if (!NQJSON_isString(json))
    return false;
  if (value)
    *value = NQJSON_asString(json);
  return true;
}

bool NQJSON_arrayGetBool(const NQJSON* json, size_t index, bool* value)
{
  const NQJSON* item = NQJSON_arrayGet(json, index);
  return jsonToBool(item, value);
}

bool NQJSON_arrayGetInt64(const NQJSON* json, size_t index, int64_t* value)
{
  const NQJSON* item = NQJSON_arrayGet(json, index);
  return jsonToInt64(item, value);
}

#ifdef NQ_USE_FLOATING_POINT
bool NQJSON_arrayGetDouble(const NQJSON* json, size_t index, double* value)
{
  const NQJSON* item = NQJSON_arrayGet(json, index);
  return jsonToDouble(item, value);
}
#endif

bool NQJSON_arrayGetString(const NQJSON* json, size_t index, const char** value)
{
  const NQJSON* item = NQJSON_arrayGet(json, index);
  return jsonToString(item, value);
}

bool NQJSON_objectGetBool(const NQJSON* json, const char* name, bool* value)
{
  const NQJSON* item = NQJSON_objectGet(json, name);
  return jsonToBool(item, value);
}

bool NQJSON_objectGetInt64(const NQJSON* json, const char* name, int64_t* value)
{
  const NQJSON* item = NQJSON_objectGet(json, name);
  return jsonToInt64(item, value);
}

#ifdef NQ_USE_FLOATING_POINT
bool NQJSON_objectGetDouble(const NQJSON* json, const char* name, double* value)
{
  const NQJSON* item = NQJSON_objectGet(json, name);
  return jsonToDouble(item, value);
}
#endif

bool NQJSON_objectGetString(const NQJSON* json, const char* name, const char** value)
{
  const NQJSON* item = NQJSON_objectGet(json, name);
  return jsonToString(item, value);
}

static bool bufferWrite(void* userdata, const char* characters, size_t length)
{
  NQStringPrint* buffer = (NQStringPrint*)userdata;
  return NQStringPrint_writeAll(buffer, characters, length);
}

bool NQJSON_dump(const NQJSON* json, NQStringPrint* buffer)
{
  NQJSONWriter writer;
  NQJSONWriter_init(&writer, bufferWrite, buffer);
  bool result = NQJSONWriter_writeJSON(&writer, (NQJSON*)json);
  NQJSONWriter_finalize(&writer);
  return result;
}
