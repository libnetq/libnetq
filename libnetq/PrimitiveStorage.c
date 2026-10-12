/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_CLASS_NAME "NQPrimitiveStorage"
#define NQ_LOG_TAG NQ_CLASS_NAME

#include "config.h"
#include "libnetq/PrimitiveStorage.h"

#include <libnetq/String.h>
#include <libnetq/Malloc.h>
#include <libnetq/Limits.h>
#include <libnetq/Log.h>

struct NQBaseValue {
  uint32_t type;
  NQListHead list;
  const char* name;
};

struct NQBoolValue {
  struct NQBaseValue base;
  bool value;
};

struct NQUint32Value {
  struct NQBaseValue base;
  uint32_t value;
};

struct NQStringValue {
  struct NQBaseValue base;
  char value[1];
};

static struct NQBaseValue* NQPrimitiveStorage_addNewValue(NQPrimitiveStorage* thiz, size_t sizeInBytes, uint32_t type, const char* name)
{
  size_t nlenz = NQStrlen(name) + 1;
  struct NQBaseValue* objVal = (struct NQBaseValue*)NQMalloc(sizeInBytes + nlenz);
  if (objVal != NULL) {
    char* ptr = (char*)objVal + sizeInBytes;
    (void)NQMemcpy(ptr, name, nlenz);
    objVal->name = ptr;
    objVal->type = type;
    NQListHead_init(&objVal->list);
    NQListHead_addBack(&thiz->entryList, &objVal->list);
  }
  return objVal;
}

static void NQPrimitiveStorage_removeNewValue(NQPrimitiveStorage* thiz, struct NQBaseValue* objVal)
{
  NQListHead_remove(&objVal->list);
  NQFree(objVal);
}

void NQPrimitiveStorage_init(NQPrimitiveStorage* thiz, NQPrimitiveStorage* parent)
{
  thiz->parent = parent;
  NQListHead_init(&thiz->entryList);
}

void NQPrimitiveStorage_finalize(NQPrimitiveStorage* thiz)
{
  while (!NQListHead_isEmpty(&thiz->entryList)) {
    struct NQBaseValue* objVal = NQ_CONTAINER_OF(thiz->entryList.prev, struct NQBaseValue, list);
    NQPrimitiveStorage_removeNewValue(thiz, objVal);
  }
}

static struct NQBaseValue* NQPrimitiveStorage_findValue(NQPrimitiveStorage* thiz, const char* name)
{
  NQListHead* iter = thiz->entryList.next;
  while (iter != &thiz->entryList) {
    struct NQBaseValue* baseVal = NQ_CONTAINER_OF(iter, struct NQBaseValue, list);
    if (!NQStrcmp(baseVal->name, name))
      return baseVal;
    iter = iter->next;
  }
  return NULL;
}

bool NQPrimitiveStorage_getBool(NQPrimitiveStorage* thiz, const char* name, bool* value)
{
  struct NQBaseValue* objVal = NQPrimitiveStorage_findValue(thiz, name);
  if (objVal != NULL) {
    if (objVal->type != kNQBoolType)
      return false;
    if (value)
      *value = ((struct NQBoolValue*)objVal)->value;
    return true;
  }

  if(thiz->parent != NULL) {
    return NQPrimitiveStorage_getBool(thiz->parent, name, value);
  }

  return false;
}

bool NQPrimitiveStorage_setBool(NQPrimitiveStorage* thiz, const char* name, bool value)
{
  struct NQBoolValue* objVal = (struct NQBoolValue*)NQPrimitiveStorage_addNewValue(thiz, sizeof(*objVal), kNQBoolType, name);
  if (objVal == NULL)
    return false;
  objVal->value = value;
  return true;
}

bool NQPrimitiveStorage_getUint32(NQPrimitiveStorage* thiz, const char* name, uint32_t* value)
{
  struct NQBaseValue* baseVal = NQPrimitiveStorage_findValue(thiz, name);
  if (baseVal != NULL) {
    if (baseVal->type != kNQUint32Type)
      return false;
    if (value)
      *value = ((struct NQUint32Value*)baseVal)->value;
    return true;
  }

  if(thiz->parent != NULL) {
    return NQPrimitiveStorage_getUint32(thiz->parent, name, value);
  }

  return false;
}

bool NQPrimitiveStorage_setUint32(NQPrimitiveStorage* thiz, const char* name, uint32_t value)
{
  struct NQBoolValue* objVal = (struct NQBoolValue*)NQPrimitiveStorage_addNewValue(thiz, sizeof(*objVal), kNQUint32Type, name);
  if (objVal == NULL)
    return false;
  objVal->value = value;
  return true;
}

bool NQPrimitiveStorage_getString(NQPrimitiveStorage* thiz, const char* name, const char** value)
{
  struct NQBaseValue* baseVal = NQPrimitiveStorage_findValue(thiz, name);
  if (baseVal != NULL) {
    if (baseVal->type != kNQStringType)
      return false;
    if (value)
      *value = ((struct NQStringValue*)baseVal)->value;
    return true;
  }

  if(thiz->parent != NULL) {
    return NQPrimitiveStorage_getString(thiz->parent, name, value);
  }

  return false;
}

bool NQPrimitiveStorage_setString(NQPrimitiveStorage* thiz, const char* name, const char* value)
{
  size_t length = NQStrlen(value);
  struct NQStringValue* objVal = (struct NQStringValue*)NQPrimitiveStorage_addNewValue(thiz, sizeof(*objVal) + length, kNQStringType, name);
  if (objVal == NULL)
    return false;

  NQMemcpy(objVal->value, value, length + 1);
  return true;
}

bool NQPrimitiveStorage_mergeJSON(NQPrimitiveStorage* storage, NQJSON* json)
{
  NQ_LOGI("Storage:");
  if (!NQJSON_isObject(json))
    return false;

  NQJSON_ObjectIter* iter = NQJSON_objectIterFirst(json);
  while (iter != NULL) {
    const char* key = NQJSON_objectIterKey(iter);
    NQJSON* value = NQJSON_objectIterValue(iter);
    if (NQJSON_isString(value)) {
      const char* val = NQJSON_asString(value);
      NQPrimitiveStorage_setString(storage, key, val);
      NQ_LOGI("  %s: '%s'", key, val);
    }
    else if (NQJSON_isInt64(value)) {
      uint32_t val = (uint32_t)NQJSON_asInt64(value); // FIXME
      NQPrimitiveStorage_setUint32(storage, key, val);
      NQ_LOGI("  %s: '%u'", key, val);
    }
    else if (NQJSON_isBool(value)) {
      bool val = NQJSON_asBool(value);
      NQPrimitiveStorage_setBool(storage, key, val);
      NQ_LOGI("  %s: '%s'", key, val ? NQ_TRUE_STRING : NQ_FALSE_STRING);
    }
    iter = NQJSON_objectIterNext(json, iter);
  }

  return true;
}
