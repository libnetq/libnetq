/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/PrimitiveType.h"

#include <libnetq/string/String.h>

bool NQStrIsFalse(const char* s)
{
  return NQStrcasecmp(s, NQ_FALSE_STRING) == 0;
}

bool NQStrIsTrue(const char* s)
{
  return NQStrcasecmp(s, NQ_TRUE_STRING) == 0;
}

bool NQStrIsOff(const char* s)
{
  return NQStrcasecmp(s, NQ_OFF_STRING) == 0;
}

bool NQStrIsOn(const char* s)
{
  return NQStrcasecmp(s, NQ_ON_STRING) == 0;
}

bool NQStrIsEnabled(const char* s)
{
  if (NQStrIsTrue(s) || NQStrIsOn(s))
    return true;
  if (!NQStrcasecmp(s, NQ_ONE_STRING))
    return true;

  return false;
}

bool NQStrIsDisabled(const char* s)
{
  if (NQStrIsFalse(s) || NQStrIsOff(s))
    return true;
  if (!NQStrcasecmp(s, NQ_ZERO_STRING))
    return true;

  return false;
}

const char* NQPrimitiveTypeToCString(NQPrimitiveType type)
{
  switch (type) {
  case kNQBoolType:
    return NQ_BOOL_STRING;
  case kNQCharType:
    return NQ_CHAR_STRING;
  case kNQInt8Type:
    return NQ_INT8_STRING;
  case kNQInt16Type:
    return NQ_INT16_STRING;
  case kNQInt32Type:
    return NQ_INT32_STRING;
  case kNQInt64Type:
    return NQ_INT64_STRING;
  case kNQUint8Type:
    return NQ_UINT8_STRING;
  case kNQUint16Type:
    return NQ_UINT16_STRING;
  case kNQUint32Type:
    return NQ_UINT32_STRING;
  case kNQUint64Type:
    return NQ_UINT64_STRING;
  case kNQFloatType:
    return NQ_FLOAT_STRING;
  case kNQDoubleType:
    return NQ_DOUBLE_STRING;
  case kNQStringType:
    return NQ_STRING_STRING;
  }

  return NULL;
}

bool NQParsePrimitiveType(const char* str, NQPrimitiveType* result)
{
  NQPrimitiveType type;

  if (!NQStrcmp(str, NQ_BOOL_STRING))
    type = kNQBoolType;
  else if (!NQStrcmp(str, NQ_CHAR_STRING))
    type = kNQCharType;
  else if (!NQStrcmp(str, NQ_INT8_STRING))
    type = kNQInt8Type;
  else if (!NQStrcmp(str, NQ_INT16_STRING))
    type = kNQInt16Type;
  else if (!NQStrcmp(str, NQ_INT32_STRING))
    type = kNQInt32Type;
  else if (!NQStrcmp(str, NQ_INT64_STRING))
    type = kNQInt64Type;
  else if (!NQStrcmp(str, NQ_UINT8_STRING))
    type = kNQUint8Type;
  else if (!NQStrcmp(str, NQ_UINT16_STRING))
    type = kNQUint16Type;
  else if (!NQStrcmp(str, NQ_UINT32_STRING))
    type = kNQUint32Type;
  else if (!NQStrcmp(str, NQ_UINT64_STRING))
    type = kNQUint64Type;
  else if (!NQStrcmp(str, NQ_FLOAT_STRING))
    type = kNQFloatType;
  else if (!NQStrcmp(str, NQ_DOUBLE_STRING))
    type = kNQDoubleType;
  else if (!NQStrcmp(str, NQ_STRING_STRING))
    type = kNQStringType;
  else
    return false;

  if (result)
    *result = type;

  return true;
}
