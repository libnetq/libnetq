/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/dbc/DBCParser.h"

#include <libnetq/CPU.h>
#include <libnetq/CType.h>
#include <libnetq/String.h>
#include <libnetq/Assert.h>
#include <libnetq/string/CStrToNum.h>
#include <libnetq/Log.h>

#define kCIdentifierMax 128

#define isSpaceSymbol(c) ((c) == ' ' || (c) == '\t' || (c) == '\v')
#define isDoubleSymbol(c) (NQIsDigit(c) || (c) == '.' || (c) == 'e' || (c) == 'E' || (c) == '+' || (c) == '-')

// TODO: cp1252 encoding

enum {
  kNQDBCSectionEmpty = -1,
};

static bool isCIdentifier(const char* s)
{
  if (!NQIsFirstIdentifier(*s))
    return false;

  while (*s) {
    if (!NQIsNextIdentifier(*s))
      return false;
    s++;
  }

  return true;
}

/*
 * C_identifier _A-Za-z _A-Za-z0-9
 * char_string
 * unsigned_integer
 * signed_integer
 * double
 * symbol  [^_A-Za-z0-9]       :|,[]()@+-;
 * finish
 */

typedef uint8_t DBCTokenType;
enum DBCTokenType {
  kCIdentifierToken,
  kCStringToken,
  kSIntegerToken,
  kUIntegerToken,
  kNumberToken,
  kSymbolToken,
};

typedef struct DBCToken {
  DBCTokenType type;
  uint32_t line;
  uint32_t column;
  uint16_t indent;
  uint32_t size;
  const char* data;
} DBCToken;

enum DBCLexerResult {
  kLexerTokenDone,
  kLexerWaitSymbol,
  kLexerNumberError,
  kLexerStringError,
  kLexerBufferError,
  kLexerInternalError,
};

enum DBCLexerMode {
  kLexerModeInit,
  kLexerModeCRLF,
  kLexerModeLF,
  kLexerModeAnySymbol,
  kLexerModeCIdentifier,
  kLexerModeCString,
  kLexerModeEscapeCString,
  kLexerModeFinalCString,
  kLexerModePlusSymbol,
  kLexerModeMinusSymbol,
  kLexerModeSInteger,
  kLexerModeUInteger,
  kLexerModeNumber,
  kLexerModeExponentSymbolNumber,
  kLexerModeExponentSignNumber,
  kLexerModeExponentNumber,
};

typedef struct DBCLexer {
  int mode;
  uint32_t currentLine;
  uint32_t currentColumn;
  uint32_t tokenLine;
  uint32_t tokenColumn;
  uint32_t indent;
  char* buffer;
  char* start;
  char* position;
  char* end;
  char prev;
} DBCLexer;

static void DBCLexer_reset(DBCLexer* thiz)
{
  thiz->mode = kLexerModeInit;
  thiz->indent = 0;
  thiz->start = thiz->buffer;
  thiz->position = thiz->buffer;
  thiz->prev = '\0';
}

static void DBCLexer_init(DBCLexer* thiz, char* buffer, size_t size)
{
  thiz->currentLine = 0;
  thiz->currentColumn = 0;

  thiz->tokenLine = 0;
  thiz->tokenColumn = 0;

  thiz->buffer = buffer;
  thiz->end = buffer + size - 1;

  DBCLexer_reset(thiz);
}

static bool DBCLexer_empty(DBCLexer* thiz)
{
  return (thiz->mode == kLexerModeInit);
}

static bool DBCLexer_apply(DBCLexer* thiz, char c)
{
  switch (thiz->mode) {
  case kLexerModeInit:
    thiz->tokenLine = thiz->currentLine;
    thiz->tokenColumn = thiz->currentColumn;

    switch (c) {
    case '\r':
      thiz->mode = kLexerModeCRLF;
      *thiz->position++ = '\n';
      return true;

    case '\n':
      thiz->mode = kLexerModeLF;
      *thiz->position++ = c;
      return true;

    case ' ':
    case '\t':
    case '\v':
      thiz->indent++;
      return true;

    case '"':
      thiz->mode = kLexerModeCString;
      return true;

    case '+':
      if (thiz->indent == 0 && NQIsDigit(thiz->prev))
        thiz->mode = kLexerModeAnySymbol;
      else
        thiz->mode = kLexerModePlusSymbol;
      *thiz->position++ = c;
      return true;

    case '-':
      if (thiz->indent == 0 && NQIsDigit(thiz->prev))
        thiz->mode = kLexerModeAnySymbol;
      else
        thiz->mode = kLexerModeMinusSymbol;
      *thiz->position++ = c;
      return true;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      thiz->mode = kLexerModeUInteger;
      *thiz->position++ = c;
      return true;
    }

    if (NQIsFirstIdentifier(c)) {
      thiz->mode = kLexerModeCIdentifier;
      *thiz->position++ = c;
    }
    else {
      thiz->mode = kLexerModeAnySymbol;
      *thiz->position++ = c;
    }

    return true;

  case kLexerModeCRLF:
    if (c == '\r' || c == '\n') {
      thiz->mode = kLexerModeLF;
      return true;
    }
    break;

  case kLexerModeLF:
    if (c == '\r') {
      thiz->mode = kLexerModeCRLF;
      return true;
    }
    if (c == '\n') {
      return true;
    }
    break;

  case kLexerModeCIdentifier:
    if (NQIsNextIdentifier(c)) {
      *thiz->position++ = c;
      return true;
    }
    break;

  case kLexerModeCString:
    if (c == '"')
      thiz->mode = kLexerModeFinalCString;
    else if (c == '\\')
      thiz->mode = kLexerModeEscapeCString;
    else
      *thiz->position++ = c;
    return true;

  case kLexerModeEscapeCString:
    thiz->mode = kLexerModeCString;
    *thiz->position++ = c;
    return true;

  case kLexerModePlusSymbol:
  case kLexerModeMinusSymbol:
    if (NQIsDigit(c)) {
      thiz->mode = kLexerModeSInteger;
      *thiz->position++ = c;
      return true;
    }
    break;

  case kLexerModeSInteger:
  case kLexerModeUInteger:
    if (NQIsDigit(c)) {
      *thiz->position++ = c;
      return true;
    }
    if (c == '.') {
      thiz->mode = kLexerModeNumber;
      *thiz->position++ = c;
      return true;
    }
    if (c == 'e' || c == 'E') {
      thiz->mode = kLexerModeExponentSymbolNumber;
      *thiz->position++ = c;
      return true;
    }
    if (thiz->mode == kLexerModeUInteger && NQIsNextIdentifier(c)) {
      thiz->mode = kLexerModeCIdentifier;
      *thiz->position++ = c;
      return true;
    }
    break;

  case kLexerModeNumber:
    if (c == 'e' || c == 'E') {
      thiz->mode = kLexerModeExponentSymbolNumber;
      *thiz->position++ = c;
      return true;
    }
    if (NQIsDigit(c)) {
      *thiz->position++ = c;
      return true;
    }
    break;

  case kLexerModeExponentSymbolNumber:
    if (c == '+' || c == '-') {
      thiz->mode = kLexerModeExponentSignNumber;
      *thiz->position++ = c;
      return true;
    }
    if (NQIsDigit(c)) {
      thiz->mode = kLexerModeExponentNumber;
      *thiz->position++ = c;
      return true;
    }
    break;

  case kLexerModeExponentSignNumber:
    if (NQIsDigit(c)) {
      thiz->mode = kLexerModeExponentNumber;
      *thiz->position++ = c;
      return true;
    }
    break;

  case kLexerModeExponentNumber:
    if (NQIsDigit(c)) {
      *thiz->position++ = c;
      return true;
    }
    break;
  }

  return false;
}

static int DBCLexer_finish(DBCLexer* thiz, struct DBCToken* token)
{
  switch (thiz->mode) {
  case kLexerModeInit:
    return kLexerWaitSymbol;

  case kLexerModeLF:
  case kLexerModeCRLF:
    token->type = kSymbolToken;
    break;

  case kLexerModeAnySymbol:
    token->type = kSymbolToken;
    break;

  case kLexerModeCIdentifier:
    token->type = kCIdentifierToken;
    break;

  case kLexerModeCString:
  case kLexerModeEscapeCString:
    return kLexerStringError;

  case kLexerModeFinalCString:
    token->type = kCStringToken;
    break;

  case kLexerModePlusSymbol:
    token->type = kSymbolToken;
    break;

  case kLexerModeMinusSymbol:
    token->type = kSymbolToken;
    break;

  case kLexerModeSInteger:
    token->type = kSIntegerToken;
    break;

  case kLexerModeUInteger:
    token->type = kUIntegerToken;
    break;

  case kLexerModeNumber:
  case kLexerModeExponentNumber:
    token->type = kNumberToken;
    break;

  case kLexerModeExponentSymbolNumber:
  case kLexerModeExponentSignNumber:
    return kLexerNumberError;

  default:
    return kLexerInternalError;
  }

  token->line = thiz->tokenLine;
  token->column = thiz->tokenColumn;
  token->indent = thiz->indent;
  token->size = (uint32_t)(thiz->position - thiz->start);

  if (token->size == 0)
    token->data = NQCStrEmpty();
  else {
    token->data = thiz->start;
    *thiz->position++ = NQ_NIL;
  }

  thiz->mode = kLexerModeInit;
  thiz->indent = 0;
  thiz->start = thiz->position;

  return kLexerTokenDone;
}

static int DBCLexer_next(DBCLexer* thiz, const char** start, const char* end, DBCToken* token)
{
  int ret = kLexerWaitSymbol;
  const char* curr = *start;

  while (curr < end) {
    char c = *curr;

    if (thiz->end <= thiz->position) {
      ret = kLexerBufferError;
      break;
    }

    if (!DBCLexer_apply(thiz, c)) {
      ret = DBCLexer_finish(thiz, token);
      break;
    }

    if (thiz->mode == kLexerModeLF) {
      thiz->currentColumn = 0;
      thiz->currentLine++;
    }
    else {
      thiz->currentColumn++;
    }

    thiz->prev = c;
    curr++;
  }

  *start = curr;
  return ret;
}

typedef uint8_t DBCStackValueType;
enum DBCStackValueType {
  kDBCValueCIdentifier,
  kDBCValueCString,
  kDBCValueInt32,
  kDBCValueUint32,
#ifdef NQ_CPU_32BIT
  kDBCValueDouble1,
  kDBCValueDouble2,
#else
  kDBCValueDouble,
#endif
};

#define kDBCStackSize 1024

#define KEYWORD_  "\x81"
#define STRING_   "\x82"
#define UINTEGER_ "\x83"
#define SINTEGER_ "\x84"
#define DOUBLE_   "\x85"
#define ATTR_     "\x86"
#define VALUE_    "\x87"
#define ENDIAN_   "\x88"
#define MPLEX_    "\x89"
#define SIGNE_    "\x8A"
#define EXTYPE_   "\x8B"
#define EVTYPE_   "\x8C"
#define ACCESS_   "\x8D"
#define CMIGNORE_ "\x8E"
#define UINTCAST_ "\x8F"
#define NOTBSKW_  "\x90"

#define _KEYWORD  (0x81)
#define _STRING   (0x82)
#define _UINTEGER (0x83)
#define _SINTEGER (0x84)
#define _DOUBLE   (0x85)
#define _ATTR     (0x86)
#define _VALUE    (0x87)
#define _ENDIAN   (0x88)
#define _MPLEX    (0x89)
#define _SIGNE    (0x8A)
#define _EXTYPE   (0x8B)
#define _EVTYPE   (0x8C)
#define _ACCESS   (0x8D)
#define _CMIGNORE (0x8E)
#define _UINTCAST (0x8F)
#define _NOTBSKW  (0x90)

typedef struct DBCMachine {
  const struct DBCOpcode* opcode;
  const char* position;

  bool done;

  uint16_t head;
  uint16_t teil;

  DBCStackValueType types[kDBCStackSize];
  uintptr_t values[kDBCStackSize];

} DBCMachine;

struct DBCOpcode {
  uint8_t next;
  char required;
  const char* pattern;
  void (*init) (DBCMachine*, NQDBCSection*);
};

#ifdef NQ_CPU_32BIT
union DoubleConvert {
  double d;
  struct {
    uint32_t v1;
    uint32_t v2;
  } s;
};
#else
union DoubleConvert {
  double d;
  uint64_t v;
};
#endif

static void DBCMachine_setOpcode(DBCMachine* thiz, const struct DBCOpcode* opcode)
{
  thiz->opcode = opcode;
  thiz->position = thiz->opcode->pattern;
  thiz->done = false;
}

static void DBCMachine_init(DBCMachine* thiz, const struct DBCOpcode* opcode)
{
  DBCMachine_setOpcode(thiz, opcode);
  thiz->head = thiz->teil = 0;
}

static inline bool DBCMachine_empty(DBCMachine* thiz)
{
  return thiz->head == thiz->teil;
}

static inline bool DBCMachine_isOverflow(DBCMachine* thiz)
{
  return kDBCStackSize <= thiz->teil;
}

static void DBCMachine_push(DBCMachine* thiz, uint8_t type, uintptr_t value)
{
  NQ_ASSERT(thiz->teil < kDBCStackSize);

  thiz->types[thiz->teil] = type;
  thiz->values[thiz->teil] = value;
  thiz->teil++;
}

static void DBCMachine_pushCIdentifier(DBCMachine* thiz, const char* value)
{
  DBCMachine_push(thiz, kDBCValueCIdentifier, (uintptr_t)value);
}

static void DBCMachine_pushCString(DBCMachine* thiz, const char* value)
{
  DBCMachine_push(thiz, kDBCValueCString, (uintptr_t)value);
}

static void DBCMachine_pushInt32(DBCMachine* thiz, int32_t value)
{
  DBCMachine_push(thiz, kDBCValueInt32, (uintptr_t)value);
}

static void DBCMachine_pushUint32(DBCMachine* thiz, uint32_t value)
{
  DBCMachine_push(thiz, kDBCValueUint32, (uintptr_t)value);
}

NQ_ALLOW_UNUSED
static void DBCMachine_pushDouble(DBCMachine* thiz, double value)
{
  union DoubleConvert u;
  u.d = value;

#ifdef NQ_CPU_32BIT
  DBCMachine_push(thiz, kDBCValueDouble1, u.s.v1);
  DBCMachine_push(thiz, kDBCValueDouble2, u.s.v2);
#else
  DBCMachine_push(thiz, kDBCValueDouble, u.v);
#endif
}

static uintptr_t DBCMachine_shift(DBCMachine* thiz, int type)
{
  NQ_ASSERT(thiz->head < thiz->teil);
  NQ_ASSERT(thiz->types[thiz->head] == type);
  (void)type;
  return thiz->values[thiz->head++];
}

static const char* DBCMachine_shiftCIdentifier(DBCMachine* thiz)
{
  return (const char*)DBCMachine_shift(thiz, kDBCValueCIdentifier);
}

static const char* DBCMachine_shiftCString(DBCMachine* thiz)
{
  return (const char*)DBCMachine_shift(thiz, kDBCValueCString);
}

static int32_t DBCMachine_shiftInt32(DBCMachine* thiz)
{
  return (int32_t)DBCMachine_shift(thiz, kDBCValueInt32);
}

static uint32_t DBCMachine_shiftUint32(DBCMachine* thiz)
{
  return (uint32_t)DBCMachine_shift(thiz, kDBCValueUint32);
}

static double DBCMachine_shiftDouble(DBCMachine* thiz)
{
  union DoubleConvert u;
#ifdef NQ_CPU_32BIT
  u.s.v1 = DBCMachine_shift(thiz, kDBCValueDouble1);
  u.s.v2 = DBCMachine_shift(thiz, kDBCValueDouble2);
#else
  u.v = DBCMachine_shift(thiz, kDBCValueDouble);
#endif
  return u.d;
}

static size_t DBCMachine_count(DBCMachine* thiz)
{
  return thiz->teil - thiz->head;
}

static const uint32_t* DBCMachine_shiftUint32List(DBCMachine* thiz, size_t* size)
{
  uint32_t* res = (uint32_t*)&thiz->values[thiz->head];
  size_t sz = DBCMachine_count(thiz);

#ifndef NQ_CPU_32BIT
  /* Fix for 64-bit */
  size_t i;
  const uintptr_t* val = &thiz->values[thiz->head];
  for (i = 0; i < sz; i++)
    res[i] = (uint32_t)val[i];
#endif

  *size = sz;
  thiz->head = thiz->teil;
  return res;
}

NQ_ALLOW_UNUSED
static const int32_t* DBCMachine_shiftInt32List(DBCMachine* thiz, size_t* size)
{
  return (const int32_t*)DBCMachine_shiftUint32List(thiz, size);
}

static const NQDBCRange* DBCMachine_shiftRangeList(DBCMachine* thiz, size_t* size)
{
  size_t sz;
  const NQDBCRange* values = (const NQDBCRange*)DBCMachine_shiftUint32List(thiz, &sz);
  *size = sz / 2;
  NQ_ASSERT(*size * 2 == sz);
  return values;
}

NQ_ALLOW_UNUSED
static const double* DBCMachine_shiftDoubleList(DBCMachine* thiz, size_t* size)
{
  const double* values = (const double*)&thiz->values[thiz->head];
  *size = DBCMachine_count(thiz);
  thiz->head = thiz->teil;
  return values;
}

static const char** DBCMachine_shiftCStringList(DBCMachine* thiz, size_t* size)
{
  const char** values = (const char**)&thiz->values[thiz->head];
  *size = DBCMachine_count(thiz);
  thiz->head = thiz->teil;
  return values;
}

static const NQDBCValDesc* DBCMachine_shiftDescriptions(DBCMachine* thiz, size_t* size)
{
  const struct NQDBCValDesc* values = (const NQDBCValDesc*)&thiz->values[thiz->head];
  size_t sz = DBCMachine_count(thiz);

#ifdef NQ_CPU_32BIT
  NQ_STATIC_ASSERT(sizeof(NQDBCValDesc[3]) == sizeof(uintptr_t) * 3 * 2, "");
#else
  NQ_STATIC_ASSERT(sizeof(NQDBCValDesc[3]) == sizeof(uintptr_t) * 3 * 2, "");
#endif

  *size = sz / 2;
  NQ_ASSERT(*size * 2 == sz);

  thiz->head = thiz->teil;
  return values;
}

static uint8_t DBCMachine_type(DBCMachine* thiz)
{
  NQ_ASSERT(thiz->head < thiz->teil);
  return thiz->types[thiz->head];
}

static bool DBCMachine_next2(DBCMachine* thiz, const DBCToken* token)
{
  char* end;
  uint32_t uinteger;
  int32_t sinteger;
  double number;

  uint8_t b = (uint8_t)*thiz->position;

  switch (b) {
  case NQ_NIL:
  case ' ':
    return false;

  case _KEYWORD:
    if (token->type != kCIdentifierToken)
      return false;
    DBCMachine_pushCIdentifier(thiz, token->data);
    thiz->position++;
    return true;

  case _STRING:
    if (token->type != kCStringToken)
      return false;
    DBCMachine_pushCString(thiz, token->data);
    thiz->position++;
    return true;

  case _UINTCAST:
    if (token->type == kUIntegerToken) {
      uinteger = NQCStrToUint32(token->data, &end, 10);
    }
    else if (token->type == kSIntegerToken) {
      uinteger = (uint32_t)NQCStrToInt32(token->data, &end, 10);
    }
    else {
      return false;
    }
    if (token->data + token->size != end)
      return false;
    DBCMachine_pushUint32(thiz, uinteger);
    thiz->position++;
    return true;

  case _UINTEGER:
    if (token->type != kUIntegerToken)
      return false;
    uinteger = NQCStrToUint32(token->data, &end, 10);
    if (token->data + token->size != end)
      return false;
    DBCMachine_pushUint32(thiz, uinteger);
    thiz->position++;
    return true;

  case _SINTEGER:
    if (token->type != kSIntegerToken && token->type != kUIntegerToken)
      return false;
    sinteger = NQCStrToInt32(token->data, &end, 10);
    if (token->data + token->size != end)
      return false;
    DBCMachine_pushInt32(thiz, sinteger);
    thiz->position++;
    return true;

  case _DOUBLE:
    if (token->type != kSIntegerToken && token->type != kUIntegerToken && token->type != kNumberToken)
      return false;
    number = strtod(token->data, &end);
    if (token->data + token->size != end)
      return false;
    DBCMachine_pushDouble(thiz, number);
    thiz->position++;
    return true;

  case _ATTR:
    if (token->type != kCStringToken || !isCIdentifier(token->data))
      return false;
    DBCMachine_pushCString(thiz, token->data);
    thiz->position++;
    return true;

  case _VALUE:
    switch (token->type) {
    case kUIntegerToken:
      uinteger = NQCStrToUint32(token->data, &end, 10);
      if (token->data + token->size != end)
        return false;
      DBCMachine_pushUint32(thiz, uinteger);
      break;

    case kSIntegerToken:
      sinteger = NQCStrToInt32(token->data, &end, 10);
      if (token->data + token->size != end)
        return false;
      DBCMachine_pushInt32(thiz, sinteger);
      break;

    case kNumberToken:
      number = strtod(token->data, &end);
      if (token->data + token->size != end)
        return false;
      DBCMachine_pushDouble(thiz, number);
      break;

    case kCStringToken:
      DBCMachine_pushCString(thiz, token->data);
      break;

    default:
      return false;
    }
    thiz->position++;
    return true;

  case _ENDIAN:
    if (token->type != kUIntegerToken)
      return false;
    NQ_ASSERT(token->size == 1);
    if (token->data[0] == '0')
      DBCMachine_pushUint32(thiz, kNQDBCBigEndian);
    else if (token->data[0] == '1')
      DBCMachine_pushUint32(thiz, kNQDBCLittleEndian);
    else
      return false;
    thiz->position++;
    return true;

  case _MPLEX:
    if (token->type != kCIdentifierToken)
      return false;
    if (token->size == 1) {
      if (token->data[0] != 'M')
        return false;
      DBCMachine_pushCIdentifier(thiz, "M");
    }
    else {
      NQ_ASSERT(token->size > 1);
      if (token->data[0] != 'm')
        return false;
      char* end;
      uint32_t value = NQCStrToUint32(token->data + 1, &end, 10);
      if (token->data + token->size == end)
        DBCMachine_pushUint32(thiz, value);
      else if (token->data + token->size - 1 == end && *end == 'M') {
        DBCMachine_pushUint32(thiz, value);
        DBCMachine_pushCIdentifier(thiz, "M");
      }
      else
        return false;
    }
    thiz->position++;
    return true;

  case _SIGNE:
    if (token->type != kSymbolToken || token->size != 1)
      return false;
    if (token->data[0] == '+')
      DBCMachine_pushUint32(thiz, 1);
    else if (token->data[0] == '-')
      DBCMachine_pushUint32(thiz, 0);
    else
      return false;
    thiz->position++;
    return true;

  case _EXTYPE:
    if (token->type != kSymbolToken || token->size != 1)
      return false;
    if (token->data[0] == '0')
      DBCMachine_pushUint32(thiz, kNQDBCSigValueInteger);
    else if (token->data[0] == '1')
      DBCMachine_pushUint32(thiz, kNQDBCSigValueFloat);
    else if (token->data[0] == '2')
      DBCMachine_pushUint32(thiz, kNQDBCSigValueDouble);
    else if (token->data[0] == '3')
      DBCMachine_pushUint32(thiz, kNQDBCSigValueUnknown);
    else
      return false;
    thiz->position++;
    return true;

  case _EVTYPE:
    if (token->type != kSymbolToken || token->size != 1)
      return false;
    if (token->data[0] == '0')
      DBCMachine_pushUint32(thiz, kNQDBCEnvValueInteger);
    else if (token->data[0] == '1')
      DBCMachine_pushUint32(thiz, kNQDBCEnvValueFloat);
    else if (token->data[0] == '2')
      DBCMachine_pushUint32(thiz, kNQDBCEnvValueString);
    else
      return false;
    thiz->position++;
    return true;

  case _ACCESS:
    if (token->type != kCIdentifierToken)
      return false;
    if (token->size <= 17 || strncmp(token->data, "DUMMY_NODE_VECTOR", 17) != 0)
      return false;
    uinteger = NQCStrToUint32(token->data + 17, &end, 16);
    if (token->data + token->size != end)
      return false;
    NQ_ASSERT((uinteger & ~(NQDBC_STRING_FLAG | NQDBC_READ_FLAG | NQDBC_WRITE_FLAG)) == 0);
    DBCMachine_pushUint32(thiz, uinteger);
    thiz->position++;
    return true;

  case _CMIGNORE:
    if (token->type == kSymbolToken && memchr(token->data, '\n', token->size) != 0)
      return false;
    thiz->position++;
    return true;

  case _NOTBSKW:
    if (token->type != kCIdentifierToken)
      return false;
    if (token->size == 3 && memcmp(token->data, "BS_", token->size) == 0)
      return false;
    DBCMachine_pushCIdentifier(thiz, token->data);
    thiz->position++;
    return true;

  default:
    if (token->type != kCIdentifierToken && token->type != kSymbolToken)
      return false;
    if (strncmp(thiz->position, token->data, token->size) != 0)
      return false;
    thiz->position += token->size;
    return true;
  }
}

static bool DBCMachine_next(DBCMachine* thiz, const DBCToken* token, bool* repeat)
{
  const struct DBCOpcode* opcode;
  NQ_ASSERT(!DBCMachine_isOverflow(thiz));

  *repeat = false;
  while (!DBCMachine_next2(thiz, token)) {
    if (thiz->opcode->pattern != thiz->position)
      return false;

    if (thiz->opcode->next != 0)
      opcode = thiz->opcode + thiz->opcode->next;
    else {
      if (thiz->opcode->required != '?' && thiz->opcode->required != '*')
        return false;

      if (thiz->opcode->init) {
        *repeat = true;
        thiz->done = true;
        break;
      }
      opcode = thiz->opcode + 1;
    }

    DBCMachine_setOpcode(thiz, opcode);
  }

  return true;
}

static bool DBCMachine_result(DBCMachine* thiz, NQDBCSection* section)
{
  if (thiz->done) {
    thiz->opcode->init(thiz, section);
    return true;
  }

  if (*thiz->position != NQ_NIL)
    return false;

  if (thiz->opcode->required == '*') {
    thiz->position = thiz->opcode->pattern;
    return false;
  }

  if (thiz->opcode->init) {
    thiz->opcode->init(thiz, section);
    return true;
  }

  DBCMachine_setOpcode(thiz, thiz->opcode + 1);
  return false;
}

static void DBCMachine_clear(DBCMachine* thiz)
{
  while (!DBCMachine_empty(thiz)) {
    int type = DBCMachine_type(thiz);
    DBCMachine_shift(thiz, type);
  }
}

#define kDBCBufferSize (4096 * 3)

struct NQDBCParserSection {
  char required;
  const char* keyword;
  int identifier;
  int errorCode;
  const struct DBCOpcode* entry;
  const struct NQDBCParserSection* childData;
  size_t childCount;
};

struct NQDBCParser {
  void* userdata;
  NQDBCParserCallback* callback;

  void* allocator;
  NQDBCAllocFn* alloc;
  NQDBCFreeFn* free;

  const struct NQDBCParserSection* parentSection;
  const struct NQDBCParserSection* currentSection;

  DBCLexer lexer;
  DBCMachine machine;

  char buffer[kDBCBufferSize];

  int errorCode;
  uint32_t line;
  uint32_t colume;

  bool hasMachine;
  uint8_t sel;
  uint32_t keywordUseMap;
};

static void initVersion(DBCMachine* thiz, NQDBCSection* section)
{
  section->version.type = kNQDBCSectionVersion;
  section->version.data = DBCMachine_shiftCString(thiz);
}

static void initNewSymbol(DBCMachine* thiz, NQDBCSection* section)
{
  section->newSymbols.type = kNQDBCSectionNewSymbols;
  section->newSymbols.data = DBCMachine_shiftCStringList(thiz, &section->newSymbols.count);
}

static void initBitTiming(DBCMachine* thiz, NQDBCSection* section)
{
  if (DBCMachine_empty(thiz))
    section->type = (enum NQDBCSectionType)kNQDBCSectionEmpty;
  else {
    section->bitTiming.type = kNQDBCSectionBitTiming;
    section->bitTiming.value.baudrate = DBCMachine_shiftUint32(thiz);
    section->bitTiming.value.btr1 = DBCMachine_shiftUint32(thiz);
    section->bitTiming.value.btr2 = DBCMachine_shiftUint32(thiz);
  }
}

static void initNodes(DBCMachine* thiz, NQDBCSection* section)
{
  section->netNodes.type = kNQDBCSectionNetNodes;
  section->netNodes.names = DBCMachine_shiftCStringList(thiz, &section->netNodes.count);
}

static void initValueTable(DBCMachine* thiz, NQDBCSection* section)
{
  section->valTable.type = kNQDBCSectionValTable;
  section->valTable.name = DBCMachine_shiftCIdentifier(thiz);
  section->valTable.valDesc = DBCMachine_shiftDescriptions(thiz, &section->valTable.count);
}

static void initMessage(DBCMachine* thiz, NQDBCSection* section)
{
  section->message.type = kNQDBCSectionMessage;
  section->message.id = DBCMachine_shiftUint32(thiz);
  section->message.name = DBCMachine_shiftCIdentifier(thiz);
  section->message.sizeInBytes = DBCMachine_shiftUint32(thiz);
  section->message.transmitter = DBCMachine_shiftCIdentifier(thiz);
}

static void initSignalNormal(DBCMachine* thiz, NQDBCSection* section)
{
  section->signal.type = kNQDBCSectionSignal;
  section->signal.name = DBCMachine_shiftCIdentifier(thiz);
  section->signal.isMultiplexorSwitch = false;
  section->signal.hasMultiplexorValue = false;
  section->signal.multiplexorValue = 0;
  section->signal.startBit = DBCMachine_shiftUint32(thiz);
  section->signal.info.sizeInBits = DBCMachine_shiftUint32(thiz);
  section->signal.info.byteOrder = (NQDBCByteOrder)DBCMachine_shiftUint32(thiz);
  section->signal.info.isUnsigned = DBCMachine_shiftUint32(thiz) ? true : false;
  section->signal.info.factor = DBCMachine_shiftDouble(thiz);
  section->signal.info.offset = DBCMachine_shiftDouble(thiz);
  section->signal.info.minimum = DBCMachine_shiftDouble(thiz);
  section->signal.info.maximum = DBCMachine_shiftDouble(thiz);
  section->signal.info.unit = DBCMachine_shiftCString(thiz);
  section->signal.receivers = DBCMachine_shiftCStringList(thiz, &section->signal.count);
}

static void initSignalMultiplexer(DBCMachine* thiz, NQDBCSection* section)
{
  section->signal.type = kNQDBCSectionSignal;
  section->signal.name = DBCMachine_shiftCIdentifier(thiz);

  section->signal.isMultiplexorSwitch = false;
  section->signal.hasMultiplexorValue = false;
  section->signal.multiplexorValue = 0;

  if (DBCMachine_type(thiz) == kDBCValueUint32) {
    section->signal.hasMultiplexorValue = true;
    section->signal.multiplexorValue = DBCMachine_shiftUint32(thiz);
  }

  if (DBCMachine_type(thiz) == kDBCValueCIdentifier) {
    section->signal.isMultiplexorSwitch = true;
    DBCMachine_shiftCIdentifier(thiz);
  }

  section->signal.startBit = DBCMachine_shiftUint32(thiz);
  section->signal.info.sizeInBits = DBCMachine_shiftUint32(thiz);
  section->signal.info.byteOrder = (NQDBCByteOrder)DBCMachine_shiftUint32(thiz);
  section->signal.info.isUnsigned = DBCMachine_shiftUint32(thiz) ? true : false;
  section->signal.info.factor = DBCMachine_shiftDouble(thiz);
  section->signal.info.offset = DBCMachine_shiftDouble(thiz);
  section->signal.info.minimum = DBCMachine_shiftDouble(thiz);
  section->signal.info.maximum = DBCMachine_shiftDouble(thiz);
  section->signal.info.unit = DBCMachine_shiftCString(thiz);
  section->signal.receivers = DBCMachine_shiftCStringList(thiz, &section->signal.count);
}

static void initMessageTransmitter(DBCMachine* thiz, NQDBCSection* section)
{
  section->messageTransmitter.type = kNQDBCSectionMessageTransmitter;
  section->messageTransmitter.messageId = DBCMachine_shiftUint32(thiz);
  section->messageTransmitter.transmitters = DBCMachine_shiftCStringList(thiz, &section->messageTransmitter.count);
}

static void initEnvVar(DBCMachine* thiz, NQDBCSection* section)
{
  section->envVar.type = kNQDBCSectionEnvVar;
  section->envVar.name = DBCMachine_shiftCIdentifier(thiz);
  section->envVar.info.valueType = (NQDBCEnvValueType)DBCMachine_shiftUint32(thiz);
  section->envVar.info.minimum = DBCMachine_shiftDouble(thiz);
  section->envVar.info.maximum = DBCMachine_shiftDouble(thiz);
  section->envVar.info.initialValue = DBCMachine_shiftDouble(thiz);
  section->envVar.info.id = DBCMachine_shiftUint32(thiz);
  section->envVar.info.accessType = (NQDBCEnvAccessType)DBCMachine_shiftUint32(thiz);
  section->envVar.accessNode = DBCMachine_shiftCStringList(thiz, &section->envVar.count);
}

static void initEnvVarData(DBCMachine* thiz, NQDBCSection* section)
{
  section->envVarData.type = kNQDBCSectionEnvVarData;
  section->envVarData.name = DBCMachine_shiftCIdentifier(thiz);
  section->envVarData.dataSize = DBCMachine_shiftUint32(thiz);
}

static void initSignalType(DBCMachine* thiz, NQDBCSection* section)
{
  section->signalType.type = kNQDBCSectionSignalType;
  section->signalType.name = DBCMachine_shiftCIdentifier(thiz);
  section->signalType.info.sizeInBits = DBCMachine_shiftUint32(thiz);
  section->signalType.info.byteOrder = (NQDBCByteOrder)DBCMachine_shiftUint32(thiz);
  section->signalType.info.isUnsigned = DBCMachine_shiftUint32(thiz) ? true : false;
  section->signalType.info.factor = DBCMachine_shiftDouble(thiz);
  section->signalType.info.offset = DBCMachine_shiftDouble(thiz);
  section->signalType.info.minimum = DBCMachine_shiftDouble(thiz);
  section->signalType.info.maximum = DBCMachine_shiftDouble(thiz);
  section->signalType.info.unit = DBCMachine_shiftCString(thiz);
  section->signalType.defaultValue = DBCMachine_shiftDouble(thiz);
  section->signalType.valTable = DBCMachine_shiftCIdentifier(thiz);
}

static void initNullComment(DBCMachine* thiz, NQDBCSection* section)
{
  section->comment.type = kNQDBCSectionComment;
  section->comment.target.objectType = kNQDBCObjectDocument;
  section->comment.value = DBCMachine_shiftCString(thiz);
  DBCMachine_clear(thiz);
}

static void initIgnoreComment(DBCMachine* thiz, NQDBCSection* section)
{
  section->comment.type = kNQDBCSectionEmpty;
  DBCMachine_clear(thiz);
}

static void initNetNodeComment(DBCMachine* thiz, NQDBCSection* section)
{
  section->comment.type = kNQDBCSectionComment;
  section->comment.target.objectType = kNQDBCObjectNetNode;
  section->comment.target.netNode.name = DBCMachine_shiftCIdentifier(thiz);
  section->comment.value = DBCMachine_shiftCString(thiz);
  DBCMachine_clear(thiz);
}

static void initMessageComment(DBCMachine* thiz, NQDBCSection* section)
{
  section->comment.type = kNQDBCSectionComment;
  section->comment.target.objectType = kNQDBCObjectMessage;
  section->comment.target.message.id = DBCMachine_shiftUint32(thiz);
  section->comment.value = DBCMachine_shiftCString(thiz);
}

static void initSignalComment(DBCMachine* thiz, NQDBCSection* section)
{
  section->comment.type = kNQDBCSectionComment;
  section->comment.target.objectType = kNQDBCObjectSignal;
  section->comment.target.signal.messageId = DBCMachine_shiftUint32(thiz);
  section->comment.target.signal.name = DBCMachine_shiftCIdentifier(thiz);
  section->comment.value = DBCMachine_shiftCString(thiz);
  DBCMachine_clear(thiz);
}

static void initEnvVarComment(DBCMachine* thiz, NQDBCSection* section)
{
  section->comment.type = kNQDBCSectionComment;
  section->comment.target.objectType = kNQDBCObjectEnvVar;
  section->comment.target.envVar.name = DBCMachine_shiftCIdentifier(thiz);
  section->comment.value = DBCMachine_shiftCString(thiz);
  DBCMachine_clear(thiz);
}

static inline void initAttrDefineInt(DBCMachine* thiz, int type, NQDBCObjectType objectType, NQDBCSection* section)
{
  section->attrDefinition.type = type;
  section->attrDefinition.objectType = objectType;
  section->attrDefinition.name = DBCMachine_shiftCString(thiz);
  section->attrDefinition.value.type = kNQDBCAttrValueInt;
  section->attrDefinition.value.vInt.first = DBCMachine_shiftInt32(thiz);
  section->attrDefinition.value.vInt.second = DBCMachine_shiftInt32(thiz);
}

static inline void initAttrDefineHex(DBCMachine* thiz, int type, NQDBCObjectType objectType, NQDBCSection* section)
{
  section->attrDefinition.type = type;
  section->attrDefinition.objectType = objectType;
  section->attrDefinition.name = DBCMachine_shiftCString(thiz);
  section->attrDefinition.value.type = kNQDBCAttrValueHex;
  section->attrDefinition.value.vHex.first = DBCMachine_shiftInt32(thiz);
  section->attrDefinition.value.vHex.second = DBCMachine_shiftInt32(thiz);
}

static inline void initAttrDefineFloat(DBCMachine* thiz, int type, NQDBCObjectType objectType, NQDBCSection* section)
{
  section->attrDefinition.type = type;
  section->attrDefinition.objectType = objectType;
  section->attrDefinition.name = DBCMachine_shiftCString(thiz);
  section->attrDefinition.value.type = kNQDBCAttrValueFloat;
  section->attrDefinition.value.vFloat.first = DBCMachine_shiftDouble(thiz);
  section->attrDefinition.value.vFloat.second = DBCMachine_shiftDouble(thiz);
}

static inline void initAttrDefineString(DBCMachine* thiz, bool isRelation, NQDBCObjectType objectType, NQDBCSection* section)
{
  section->attrDefinition.type = isRelation ? kNQDBCSectionAttrRelDefinition : kNQDBCSectionAttrDefinition;
  section->attrDefinition.objectType = objectType;
  section->attrDefinition.name = DBCMachine_shiftCString(thiz);
  section->attrDefinition.value.type = kNQDBCAttrValueString;

  if (isRelation) {
    DBCMachine_shiftCString(thiz);
  }
}

static inline void initAttrDefineEnum(DBCMachine* thiz, int type, NQDBCObjectType objectType, NQDBCSection* section)
{
  section->attrDefinition.type = type;
  section->attrDefinition.objectType = objectType;
  section->attrDefinition.name = DBCMachine_shiftCString(thiz);
  section->attrDefinition.value.type = kNQDBCAttrValueEnum;
  section->attrDefinition.value.vEnum.data = DBCMachine_shiftCStringList(thiz, &section->attrDefinition.value.vEnum.count);
}

static void initAttrDefineNullInt(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineInt(thiz, kNQDBCSectionAttrDefinition, kNQDBCObjectDocument, section);
}

NQ_ALLOW_UNUSED
static void initAttrRelDefineNullInt(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineInt(thiz, kNQDBCSectionAttrRelDefinition, kNQDBCObjectDocument, section);
}

static void initAttrDefineNullHex(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineHex(thiz, kNQDBCSectionAttrDefinition, kNQDBCObjectDocument, section);
}

NQ_ALLOW_UNUSED
static void initAttrRelDefineNullHex(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineHex(thiz, kNQDBCSectionAttrRelDefinition, kNQDBCObjectDocument, section);
}

static void initAttrDefineNullFloat(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineFloat(thiz, kNQDBCSectionAttrDefinition, kNQDBCObjectDocument, section);
}

NQ_ALLOW_UNUSED
static void initAttrRelDefineNullFloat(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineFloat(thiz, kNQDBCSectionAttrRelDefinition, kNQDBCObjectDocument, section);
}

static void initAttrDefineNullString(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineString(thiz, false, kNQDBCObjectDocument, section);
}

NQ_ALLOW_UNUSED
static void initAttrRelDefineNullString(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineString(thiz, true, kNQDBCObjectDocument, section);
}

static void initAttrDefineNullEnum(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineEnum(thiz, kNQDBCSectionAttrDefinition, kNQDBCObjectDocument, section);
}

static void initAttrDefineNodeInt(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineInt(thiz, kNQDBCSectionAttrDefinition, kNQDBCObjectNetNode, section);
}

static void initAttrDefineNodeHex(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineHex(thiz, kNQDBCSectionAttrDefinition, kNQDBCObjectNetNode, section);
}

static void initAttrDefineNodeFloat(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineFloat(thiz, kNQDBCSectionAttrDefinition, kNQDBCObjectNetNode, section);
}

static void initAttrDefineNodeString(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineString(thiz, false, kNQDBCObjectNetNode, section);
}

static void initAttrDefineNodeEnum(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineEnum(thiz, kNQDBCSectionAttrDefinition, kNQDBCObjectNetNode, section);
}

static void initAttrDefineMessageInt(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineInt(thiz, kNQDBCSectionAttrDefinition, kNQDBCObjectMessage, section);
}

static void initAttrRelDefineMessageInt(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineInt(thiz, kNQDBCSectionAttrRelDefinition, kNQDBCObjectMessage, section);
}

static void initAttrDefineMessageHex(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineHex(thiz, kNQDBCSectionAttrDefinition, kNQDBCObjectMessage, section);
}

static void initAttrRelDefineMessageHex(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineHex(thiz, kNQDBCSectionAttrRelDefinition, kNQDBCObjectMessage, section);
}

static void initAttrDefineMessageFloat(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineFloat(thiz, kNQDBCSectionAttrDefinition, kNQDBCObjectMessage, section);
}

static void initAttrRelDefineMessageFloat(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineFloat(thiz, kNQDBCSectionAttrRelDefinition, kNQDBCObjectMessage, section);
}

static void initAttrDefineMessageString(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineString(thiz, false, kNQDBCObjectMessage, section);
}

static void initAttrRelDefineMessageString(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineString(thiz, true, kNQDBCObjectMessage, section);
}

static void initAttrDefineMessageEnum(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineEnum(thiz, kNQDBCSectionAttrDefinition, kNQDBCObjectMessage, section);
}

static void initAttrRelDefineMessageEnum(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineEnum(thiz, kNQDBCSectionAttrRelDefinition, kNQDBCObjectMessage, section);
}

static void initAttrDefineSignalInt(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineInt(thiz, kNQDBCSectionAttrDefinition, kNQDBCObjectSignal, section);
}

static void initAttrRelDefineSignalInt(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineInt(thiz, kNQDBCSectionAttrRelDefinition, kNQDBCObjectSignal, section);
}

static void initAttrDefineSignalHex(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineHex(thiz, kNQDBCSectionAttrDefinition, kNQDBCObjectSignal, section);
}

static void initAttrRelDefineSignalHex(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineHex(thiz, kNQDBCSectionAttrRelDefinition, kNQDBCObjectSignal, section);
}

static void initAttrDefineSignalFloat(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineFloat(thiz, kNQDBCSectionAttrDefinition, kNQDBCObjectSignal, section);
}

static void initAttrRelDefineSignalFloat(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineFloat(thiz, kNQDBCSectionAttrRelDefinition, kNQDBCObjectSignal, section);
}

static void initAttrDefineSignalString(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineString(thiz, false, kNQDBCObjectSignal, section);
}

static void initAttrRelDefineSignalString(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineString(thiz, true, kNQDBCObjectSignal, section);
}

static void initAttrDefineSignalEnum(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineEnum(thiz, kNQDBCSectionAttrDefinition, kNQDBCObjectSignal, section);
}

static void initAttrRelDefineSignalEnum(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineEnum(thiz, kNQDBCSectionAttrRelDefinition, kNQDBCObjectSignal, section);
}

static void initAttrDefineEnvVarInt(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineInt(thiz, kNQDBCSectionAttrDefinition, kNQDBCObjectEnvVar, section);
}

static void initAttrRelDefineEnvVarInt(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineInt(thiz, kNQDBCSectionAttrRelDefinition, kNQDBCObjectEnvVar, section);
}

static void initAttrDefineEnvVarHex(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineHex(thiz, kNQDBCSectionAttrDefinition, kNQDBCObjectEnvVar, section);
}

static void initAttrRelDefineEnvVarHex(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineHex(thiz, kNQDBCSectionAttrRelDefinition, kNQDBCObjectEnvVar, section);
}

static void initAttrDefineEnvVarFloat(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineFloat(thiz, kNQDBCSectionAttrDefinition, kNQDBCObjectEnvVar, section);
}

static void initAttrRelDefineEnvVarFloat(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineFloat(thiz, kNQDBCSectionAttrRelDefinition, kNQDBCObjectEnvVar, section);
}

static void initAttrDefineEnvVarString(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineString(thiz, false, kNQDBCObjectEnvVar, section);
}

static void initAttrRelDefineEnvVarString(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineString(thiz, true, kNQDBCObjectEnvVar, section);
}

static void initAttrDefineEnvVarEnum(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineEnum(thiz, kNQDBCSectionAttrDefinition, kNQDBCObjectEnvVar, section);
}

static void initAttrRelDefineEnvVarEnum(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefineEnum(thiz, kNQDBCSectionAttrRelDefinition, kNQDBCObjectEnvVar, section);
}

static void DBCMachineSetAttrValue(DBCMachine* thiz, NQDBCVariant* value)
{
  switch (DBCMachine_type(thiz)) {
  case kDBCValueUint32:
    value->type = kNQDBCVariantUint;
    value->vInt = (int64_t)DBCMachine_shiftUint32(thiz);
    break;
  case kDBCValueInt32:
    value->type = kNQDBCVariantInt;
    value->vInt = (int64_t)DBCMachine_shiftInt32(thiz);
    break;
  case kDBCValueCString:
    value->type = kNQDBCVariantString;
    value->vString = DBCMachine_shiftCString(thiz);
    break;
  default:
    value->type = kNQDBCVariantFloat;
    value->vFloat = DBCMachine_shiftDouble(thiz);
    break;
  }
}

static inline void initAttrDefaultBase(DBCMachine* thiz, int type, NQDBCSection* section)
{
  section->attrDefault.type = type;
  section->attrDefault.name = DBCMachine_shiftCString(thiz);

  DBCMachineSetAttrValue(thiz, &section->attrDefault.value);
}

static void initAttrDefault(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefaultBase(thiz, kNQDBCSectionAttrDefault, section);
}

static void initAttrRelDefault(DBCMachine* thiz, NQDBCSection* section)
{
  initAttrDefaultBase(thiz, kNQDBCSectionAttrRelDefault, section);
}

static void initAttrNull(DBCMachine* thiz, NQDBCSection* section)
{
  section->attr.type = kNQDBCSectionAttr;
  section->attr.name = DBCMachine_shiftCString(thiz);
  section->attr.target.objectType = kNQDBCObjectDocument;

  DBCMachineSetAttrValue(thiz, &section->attr.value);
}

static void initAttrNode(DBCMachine* thiz, NQDBCSection* section)
{
  section->attr.type = kNQDBCSectionAttr;
  section->attr.name = DBCMachine_shiftCString(thiz);
  section->attr.target.objectType = kNQDBCObjectNetNode;
  section->attr.target.netNode.name = DBCMachine_shiftCIdentifier(thiz);

  DBCMachineSetAttrValue(thiz, &section->attr.value);
}

static void initAttrMessage(DBCMachine* thiz, NQDBCSection* section)
{
  section->attr.type = kNQDBCSectionAttr;
  section->attr.name = DBCMachine_shiftCString(thiz);
  section->attr.target.objectType = kNQDBCObjectMessage;
  section->attr.target.message.id = DBCMachine_shiftUint32(thiz);

  DBCMachineSetAttrValue(thiz, &section->attr.value);
}

static void initAttrRelMessage(DBCMachine* thiz, NQDBCSection* section)
{
  section->attrRel.type = kNQDBCSectionAttrRel;
  section->attrRel.name = DBCMachine_shiftCString(thiz);
  section->attrRel.netNode = DBCMachine_shiftCIdentifier(thiz);
  section->attrRel.target.objectType = kNQDBCObjectMessage;
  section->attrRel.target.message.id = DBCMachine_shiftUint32(thiz);

  DBCMachineSetAttrValue(thiz, &section->attrRel.value);
}

static void initAttrSignal(DBCMachine* thiz, NQDBCSection* section)
{
  section->attr.type = kNQDBCSectionAttr;
  section->attr.name = DBCMachine_shiftCString(thiz);
  section->attr.target.objectType = kNQDBCObjectSignal;
  section->attr.target.signal.messageId = DBCMachine_shiftUint32(thiz);
  section->attr.target.signal.name = DBCMachine_shiftCIdentifier(thiz);

  DBCMachineSetAttrValue(thiz, &section->attr.value);
}

static void initAttrRelSignal(DBCMachine* thiz, NQDBCSection* section)
{
  section->attrRel.type = kNQDBCSectionAttrRel;
  section->attrRel.name = DBCMachine_shiftCString(thiz);
  section->attrRel.netNode = DBCMachine_shiftCIdentifier(thiz);
  section->attrRel.target.objectType = kNQDBCObjectSignal;
  section->attrRel.target.signal.messageId = DBCMachine_shiftUint32(thiz);
  section->attrRel.target.signal.name = DBCMachine_shiftCIdentifier(thiz);

  DBCMachineSetAttrValue(thiz, &section->attrRel.value);
}

static void initAttrEnvVar(DBCMachine* thiz, NQDBCSection* section)
{
  section->attr.type = kNQDBCSectionAttr;
  section->attr.name = DBCMachine_shiftCString(thiz);
  section->attr.target.objectType = kNQDBCObjectEnvVar;
  section->attr.target.envVar.name = DBCMachine_shiftCIdentifier(thiz);

  DBCMachineSetAttrValue(thiz, &section->attr.value);
}

static void initAttrRelEnvVar(DBCMachine* thiz, NQDBCSection* section)
{
  section->attrRel.type = kNQDBCSectionAttrRel;
  section->attrRel.name = DBCMachine_shiftCString(thiz);
  section->attrRel.netNode = DBCMachine_shiftCIdentifier(thiz);
  section->attrRel.target.objectType = kNQDBCObjectEnvVar;
  section->attrRel.target.envVar.name = DBCMachine_shiftCIdentifier(thiz);

  DBCMachineSetAttrValue(thiz, &section->attrRel.value);
}

static void initSignalTypeRef(DBCMachine* thiz, NQDBCSection* section)
{
  section->signalTypeRef.type = kNQDBCSectionSignalTypeRef;
  section->signalTypeRef.messageId = DBCMachine_shiftUint32(thiz);
  section->signalTypeRef.signalName = DBCMachine_shiftCIdentifier(thiz);
  section->signalTypeRef.signalTypeName = DBCMachine_shiftCIdentifier(thiz);
}

static void initSignalGroup(DBCMachine* thiz, NQDBCSection* section)
{
  section->signalGroup.type = kNQDBCSectionSignalGroup;
  section->signalGroup.messageId = DBCMachine_shiftUint32(thiz);
  section->signalGroup.name = DBCMachine_shiftCIdentifier(thiz);
  section->signalGroup.repetitions = DBCMachine_shiftUint32(thiz);
  section->signalGroup.signals = DBCMachine_shiftCStringList(thiz, &section->signalGroup.count);
}

static void initSignalValueType(DBCMachine* thiz, NQDBCSection* section)
{
  section->signalValueType.type = kNQDBCSectionSignalValueType;
  section->signalValueType.messageId = DBCMachine_shiftUint32(thiz);
  section->signalValueType.signalName = DBCMachine_shiftCIdentifier(thiz);
  section->signalValueType.value = (NQDBCSigValueType)DBCMachine_shiftUint32(thiz);
}

static void initSignalValue(DBCMachine* thiz, NQDBCSection* section)
{
  section->signalValue.type = kNQDBCSectionSignalValue;
  section->signalValue.messageId = DBCMachine_shiftUint32(thiz);
  section->signalValue.signalName = DBCMachine_shiftCIdentifier(thiz);
  section->signalValue.valDesc = DBCMachine_shiftDescriptions(thiz, &section->signalValue.count);
}

static void initValueEnvVar(DBCMachine* thiz, NQDBCSection* section)
{
  section->envVarValue.type = kNQDBCSectionEnvVarValue;
  section->envVarValue.name = DBCMachine_shiftCIdentifier(thiz);
  section->envVarValue.valDesc = DBCMachine_shiftDescriptions(thiz, &section->envVarValue.count);
}

static void initExMultiplex(DBCMachine* thiz, NQDBCSection* section)
{
  section->signalMultiplexed.type = kNQDBCSectionSignalMultiplexed;
  section->signalMultiplexed.messageId = DBCMachine_shiftUint32(thiz);
  section->signalMultiplexed.signalName = DBCMachine_shiftCIdentifier(thiz);
  section->signalMultiplexed.switchName = DBCMachine_shiftCIdentifier(thiz);
  section->signalMultiplexed.range = DBCMachine_shiftRangeList(thiz, &section->signalMultiplexed.count);
}

static const struct DBCOpcode s_version[] = {
  { 0, '1', STRING_, &initVersion }
};

static const struct DBCOpcode s_new_symbols[] = {
  { 0, '1', ":\n" },
  { 0, '*', NOTBSKW_ "\n", &initNewSymbol}
};

static const struct DBCOpcode s_bit_timing[] = {
  { 0, '1', ":" },
  { 0, '?', UINTEGER_ ":" UINTEGER_ "," UINTEGER_, &initBitTiming}
};

static const struct DBCOpcode s_nodes[] = {
  { 0, '1', ":" },
  { 0, '*', KEYWORD_, &initNodes }
};

static const struct DBCOpcode s_value_tables[] = {
  { 0, '1', KEYWORD_ },
  { 0, '*', SINTEGER_ STRING_ },
  { 0, '1', ";", &initValueTable }
};

static const struct DBCOpcode s_messages[] = {
  { 0, '1', UINTEGER_ KEYWORD_ ":" UINTEGER_ KEYWORD_, &initMessage }
};

static const struct DBCOpcode s_signal[] = {
  { 0, '1', KEYWORD_ },
  { 2, '1', ":" UINTEGER_ "|" UINTEGER_ "@" ENDIAN_ SIGNE_ "(" DOUBLE_ "," DOUBLE_ ")" "[" DOUBLE_  "|" DOUBLE_ "]" STRING_ KEYWORD_ },
  { 0, '*', "," KEYWORD_, &initSignalNormal },
  { 0, '1', MPLEX_ ":" UINTEGER_ "|" UINTEGER_ "@" ENDIAN_ SIGNE_ "(" DOUBLE_ "," DOUBLE_ ")" "[" DOUBLE_ "|" DOUBLE_ "]" STRING_ KEYWORD_ },
  { 0, '*', "," KEYWORD_, &initSignalMultiplexer },
};

static const struct DBCOpcode s_message_transmitters[] = {
  { 0, '1', UINTEGER_ ":" KEYWORD_ },
  { 0, '*', "," KEYWORD_ },
  { 0, '1', ";", &initMessageTransmitter }
};

static const struct DBCOpcode s_environment_variables[] = {
  { 0, '1', KEYWORD_ ":" EVTYPE_ "[" DOUBLE_ "|" DOUBLE_ "]" STRING_ DOUBLE_ UINTEGER_ ACCESS_ KEYWORD_ },
  { 0, '*', "," KEYWORD_ },
  { 0, '1', ";", &initEnvVar }
};

static const struct DBCOpcode s_environment_variables_data[] = {
  { 0, '1', KEYWORD_ ":" UINTEGER_ ";", &initEnvVarData }
};

static const struct DBCOpcode s_signal_types[] = {
  { 1, '1', KEYWORD_ ":" UINTEGER_ "@" ENDIAN_ SIGNE_ "(" DOUBLE_ "," DOUBLE_ ")" "[" DOUBLE_ "|" DOUBLE_ "]" STRING_ DOUBLE_ "," KEYWORD_ ";", &initSignalType},
  { 0, '1', UINTEGER_ KEYWORD_ ": " KEYWORD_ ";", &initSignalTypeRef },
};

static const struct DBCOpcode s_signal_type_refs[] = {
  { 0, '1', UINTEGER_ KEYWORD_ ":" KEYWORD_ ";", &initSignalTypeRef },
};

static const struct DBCOpcode s_comments[] = {
  {  5, '1', "BU_" },
  { 23, '1', KEYWORD_ },
  { 22, '1', STRING_ },
  {  0, '?', ";" },
  {  0, '*', CMIGNORE_, &initNetNodeComment },

  {  5, '1', "BO_" },
  { 18, '1', UINTEGER_ },
  { 17, '1', STRING_ },
  {  0, '?', ";" },
  {  0, '*', CMIGNORE_, &initMessageComment },

  {  6, '1', "SG_" },
  { 13, '1', UINTEGER_ },
  { 12, '1', KEYWORD_ },
  { 11, '1', STRING_ },
  {  0, '?', ";" },
  {  0, '*', CMIGNORE_, &initSignalComment },

  {  5, '1', "EV_" },
  {  7, '1', KEYWORD_ },
  {  6, '1', STRING_ },
  {  0, '?', ";" },
  {  0, '*', CMIGNORE_, &initEnvVarComment },

  {  3, '1', STRING_ },
  {  0, '?', ";" },
  {  0, '*', CMIGNORE_, &initNullComment },

  {  0, '*', CMIGNORE_, &initIgnoreComment },
};

static const struct DBCOpcode s_attribute_definitions[] = {
  { 8, '1', "BU_" ATTR_ },
  { 1, '1', "INT" SINTEGER_ SINTEGER_ ";", &initAttrDefineNodeInt },
  { 1, '1', "HEX" SINTEGER_ SINTEGER_ ";", &initAttrDefineNodeHex },
  { 1, '1', "FLOAT" DOUBLE_ DOUBLE_ ";", &initAttrDefineNodeFloat },
  { 1, '1', "STRING;", &initAttrDefineNodeString },
  { 0, '1', "ENUM" STRING_ },
  { 0, '*', "," STRING_ },
  { 0, '1', ";", &initAttrDefineNodeEnum },
  { 8, '1', "BO_" ATTR_ },
  { 1, '1', "INT" SINTEGER_ SINTEGER_ ";", &initAttrDefineMessageInt },
  { 1, '1', "HEX" SINTEGER_ SINTEGER_ ";", &initAttrDefineMessageHex },
  { 1, '1', "FLOAT" DOUBLE_ DOUBLE_ ";", &initAttrDefineMessageFloat },
  { 1, '1', "STRING;", &initAttrDefineMessageString },
  { 0, '1', "ENUM" STRING_ },
  { 0, '*', "," STRING_ },
  { 0, '1', ";", &initAttrDefineMessageEnum },
  { 8, '1', "SG_" ATTR_ },
  { 1, '1', "INT" SINTEGER_ SINTEGER_ ";", &initAttrDefineSignalInt },
  { 1, '1', "HEX" SINTEGER_ SINTEGER_ ";", &initAttrDefineSignalHex },
  { 1, '1', "FLOAT" DOUBLE_ DOUBLE_ ";", &initAttrDefineSignalFloat },
  { 1, '1', "STRING;", &initAttrDefineSignalString },
  { 0, '1', "ENUM" STRING_ },
  { 0, '*', "," STRING_ },
  { 0, '1', ";", &initAttrDefineSignalEnum },
  { 8, '1', "EV_" ATTR_ },
  { 1, '1', "INT" SINTEGER_ SINTEGER_ ";", &initAttrDefineEnvVarInt },
  { 1, '1', "HEX" SINTEGER_ SINTEGER_ ";", &initAttrDefineEnvVarHex },
  { 1, '1', "FLOAT" DOUBLE_ DOUBLE_ ";", &initAttrDefineEnvVarFloat },
  { 1, '1', "STRING;", &initAttrDefineEnvVarString },
  { 0, '1', "ENUM" STRING_ },
  { 0, '*', "," STRING_ },
  { 0, '1', ";", &initAttrDefineEnvVarEnum },
  { 0, '1', ATTR_ },
  { 1, '1', "INT" SINTEGER_ SINTEGER_ ";", &initAttrDefineNullInt },
  { 1, '1', "HEX" SINTEGER_ SINTEGER_ ";", &initAttrDefineNullHex },
  { 1, '1', "FLOAT" DOUBLE_ DOUBLE_ ";", &initAttrDefineNullFloat },
  { 1, '1', "STRING;", &initAttrDefineNullString },
  { 0, '1', "ENUM" STRING_ },
  { 0, '*', "," STRING_ },
  { 0, '1', ";", &initAttrDefineNullEnum },
};

static const struct DBCOpcode s_attribute_relation_definitions[] = {
  { 8, '1', "BU_BO_REL_" ATTR_ },
  { 1, '1', "INT" SINTEGER_ SINTEGER_ ";", &initAttrRelDefineMessageInt },
  { 1, '1', "HEX" SINTEGER_ SINTEGER_ ";", &initAttrRelDefineMessageHex },
  { 1, '1', "FLOAT" DOUBLE_ DOUBLE_ ";", &initAttrRelDefineMessageFloat },
  { 1, '1', "STRING" STRING_ ";", &initAttrRelDefineMessageString },
  { 0, '1', "ENUM" STRING_ },
  { 0, '*', "," STRING_ },
  { 0, '1', ";", &initAttrRelDefineMessageEnum },
  { 8, '1', "BU_SG_REL_" ATTR_ },
  { 1, '1', "INT" SINTEGER_ SINTEGER_ ";", &initAttrRelDefineSignalInt },
  { 1, '1', "HEX" SINTEGER_ SINTEGER_ ";", &initAttrRelDefineSignalHex },
  { 1, '1', "FLOAT" DOUBLE_ DOUBLE_ ";", &initAttrRelDefineSignalFloat },
  { 1, '1', "STRING" STRING_ ";", &initAttrRelDefineSignalString },
  { 0, '1', "ENUM" STRING_ },
  { 0, '*', "," STRING_ },
  { 0, '1', ";", &initAttrRelDefineSignalEnum },
  { 0, '1', "BU_EV_REL_" ATTR_ },
  { 1, '1', "INT" SINTEGER_ SINTEGER_ ";", &initAttrRelDefineEnvVarInt },
  { 1, '1', "HEX" SINTEGER_ SINTEGER_ ";", &initAttrRelDefineEnvVarHex },
  { 1, '1', "FLOAT" DOUBLE_ DOUBLE_ ";", &initAttrRelDefineEnvVarFloat },
  { 1, '1', "STRING" STRING_ ";", &initAttrRelDefineEnvVarString },
  { 0, '1', "ENUM" STRING_ },
  { 0, '*', "," STRING_ },
  { 0, '1', ";", &initAttrRelDefineEnvVarEnum },
};

static const struct DBCOpcode s_attribute_defaults[] = {
  { 0, '1', ATTR_ VALUE_ ";", &initAttrDefault },
};

static const struct DBCOpcode s_attribute_relation_defaults[] = {
  { 0, '1', ATTR_ VALUE_ ";", &initAttrRelDefault },
};

static const struct DBCOpcode s_attribute_values[] = {
  { 0, '1', ATTR_ },
  { 1, '1', "BU_" KEYWORD_ VALUE_ ";", &initAttrNode },
  { 1, '1', "BO_" UINTEGER_ VALUE_ ";", &initAttrMessage },
  { 1, '1', "SG_" UINTEGER_ KEYWORD_ VALUE_ ";", &initAttrSignal },
  { 1, '1', "EV_" KEYWORD_ VALUE_ ";", &initAttrEnvVar },
  { 0, '1', VALUE_ ";", &initAttrNull },
};

static const struct DBCOpcode s_attribute_relation_values[] = {
  { 0, '1', ATTR_ },
  { 1, '1', "BU_BO_REL_" KEYWORD_ UINTEGER_ VALUE_ ";", &initAttrRelMessage },
  { 1, '1', "BU_SG_REL_" KEYWORD_ "SG_" UINTEGER_ KEYWORD_ VALUE_ ";", &initAttrRelSignal },
  { 0, '1', "BU_EV_REL_" KEYWORD_ KEYWORD_ VALUE_ ";", &initAttrRelEnvVar },
};

static const struct DBCOpcode s_value_descriptions[] = {
  { 3, '1', UINTEGER_ KEYWORD_ UINTCAST_ STRING_ },
  { 0, '*', UINTCAST_ STRING_ },
  { 0, '?', ";", &initSignalValue },
  { 0, '1', KEYWORD_ UINTCAST_ STRING_ },
  { 0, '*', UINTCAST_ STRING_ },
  { 0, '?', ";", &initValueEnvVar },
};

static const struct DBCOpcode s_signal_groups[] = {
  { 0, '1', UINTEGER_ KEYWORD_ UINTEGER_ ":" },
  { 0, '*', KEYWORD_ },
  { 0, '1', ";", &initSignalGroup },
};

static const struct DBCOpcode s_signal_extended_value_type_list[] = {
  { 0, '1', UINTEGER_ KEYWORD_ EXTYPE_ ";", &initSignalValueType },
};

static const struct DBCOpcode extended_multiplexing[] = {
  { 0, '1', UINTEGER_ KEYWORD_ KEYWORD_ UINTEGER_ "-" UINTEGER_ },
  { 0, '*', "," UINTEGER_ "-" UINTEGER_ },
  { 0, '1', ";", &initExMultiplex },
};

static const struct NQDBCParserSection s_message_child_sections[] = {
  // signals
  { '*', "SG_", NQDBC_KEYWORD_SG, kNQDBCErrorSectionSignal, s_signal, NULL },
  // comments
  { '*', "CM_", NQDBC_KEYWORD_CM, kNQDBCErrorSectionComment, s_comments, NULL },
};

static const struct NQDBCParserSection s_sections[] = {
  // version
  { '?', "VERSION", NQDBC_KEYWORD_VERSION, kNQDBCErrorSectionVersion, s_version, NULL, 0 },
  // new_symbols
  { '?', "NS_", NQDBC_KEYWORD_NS, kNQDBCErrorSectionNewSymbols, s_new_symbols, NULL, 0 },
  // bit_timing (*obsolete but required*)
  { '1', "BS_", NQDBC_KEYWORD_BS, kNQDBCErrorSectionBitTiming, s_bit_timing, NULL, 0 },
  // nodes
  { '1', "BU_", NQDBC_KEYWORD_BU, kNQDBCErrorSectionNetNodes, s_nodes, NULL, 0 },
  // value_tables
  { '*', "VAL_TABLE_", NQDBC_KEYWORD_VAL_TABLE, kNQDBCErrorSectionValTable, s_value_tables, NULL, 0 },
  // messages
  { '*', "BO_", NQDBC_KEYWORD_BO, kNQDBCErrorSectionMessage, s_messages, s_message_child_sections, NQ_ARRAY_LENGTH(s_message_child_sections) },
  // message_transmitters
  { '*', "BO_TX_BU_", NQDBC_KEYWORD_BO_TX_BU, kNQDBCErrorSectionMessageTransmitter, s_message_transmitters, NULL, 0 },
  // environment_variables
  { '*', "EV_", NQDBC_KEYWORD_EV, kNQDBCErrorSectionEnvVar, s_environment_variables, NULL, 0 },
  // environment_variables_data
  { '*', "ENVVAR_DATA_", NQDBC_KEYWORD_ENVVAR_DATA, kNQDBCErrorSectionEnvVarData, s_environment_variables_data, NULL, 0 },
  // signal_types (*private*)
  { '*', "SGTYPE_", NQDBC_KEYWORD_SGTYPE, kNQDBCErrorSectionSignalType, s_signal_types, NULL, 0 },
  // comments
  { '*', "CM_", NQDBC_KEYWORD_CM, kNQDBCErrorSectionComment, s_comments, NULL, 0 },
  // attribute_definitions
  { '*', "BA_DEF_", NQDBC_KEYWORD_BA_DEF, kNQDBCErrorSectionAttrDefinition, s_attribute_definitions, NULL, 0 },
  // attribute_relation_definitions
  { '*', "BA_DEF_REL_", NQDBC_KEYWORD_BA_DEF_REL, kNQDBCErrorSectionAttrRelDefinition, s_attribute_relation_definitions, NULL, 0 },
  // sigtype_attr_list (*private*)
  // attribute_defaults
  { '*', "BA_DEF_DEF_", NQDBC_KEYWORD_BA_DEF_DEF, kNQDBCErrorSectionAttrDefault, s_attribute_defaults, NULL, 0 },
  // attribute_relation_defaults
  { '*', "BA_DEF_DEF_REL_", NQDBC_KEYWORD_BA_DEF_DEF_REL, kNQDBCErrorSectionAttrRelDefault, s_attribute_relation_defaults, NULL, 0 },
  // attribute_values
  { '*', "BA_", NQDBC_KEYWORD_BA, kNQDBCErrorSectionAttrValue, s_attribute_values, NULL, 0 },
  // attribute_relation_values
  { '*', "BA_REL_", NQDBC_KEYWORD_BA_REL, kNQDBCErrorSectionAttrRelValue, s_attribute_relation_values, NULL, 0 },
  // value_descriptions
  { '*', "VAL_", NQDBC_KEYWORD_VAL, kNQDBCErrorSectionValueDescription, s_value_descriptions, NULL, 0 },
  // category_definitions (*obsolete*)
  // categories (*obsolete*)
  // filter (*obsolete*)
  // signal_type_refs (*private*)
  // TODO: Del { '*', "SGTYPE_", NQDBC_KEYWORD_SIG_TYPE_REF, kNQDBCErrorSectionSignalTypeRef, s_signal_type_refs, NULL, 0 },
  { '*', "SIG_TYPE_REF_", NQDBC_KEYWORD_SIG_TYPE_REF, kNQDBCErrorSectionSignalTypeRef, s_signal_type_refs, NULL, 0 },
  // signal_groups
  { '*', "SIG_GROUP_", NQDBC_KEYWORD_SIG_GROUP, kNQDBCErrorSectionSignalGroup, s_signal_groups, NULL, 0 },
  // signal_extended_value_type_list (*private*)
  { '*', "SIG_VALTYPE_", NQDBC_KEYWORD_SIG_VALTYPE, kNQDBCErrorSectionSignalValueType, s_signal_extended_value_type_list, NULL, 0 },
  // extended_multiplexing
  { '*', "SG_MUL_VAL_", NQDBC_KEYWORD_SG_MUL_VAL, kNQDBCErrorSectionMultiplexedSignal, extended_multiplexing, NULL, 0 },
};

static bool isKeywordEqual(const char* keyword, const char* data, size_t size) {
  size_t index = 0;
  for (;;) {
    char c = *keyword;
    if (c == NQ_NIL)
      return (index == size);
    if (size <= index || c != data[index])
      return false;
    keyword++;
    index++;
  }
}

static bool NQDBCParser_chooseSection(NQDBCParser* thiz, const char* tokenData, size_t tokenSize)
{
  if (isKeywordEqual(thiz->currentSection->keyword, tokenData, tokenSize)) {
    return true;
  }

  if (thiz->parentSection) {
    for (size_t index = 0; index < thiz->parentSection->childCount; index++) {
      const struct NQDBCParserSection* iter = &thiz->parentSection->childData[index];
      if (isKeywordEqual(iter->keyword, tokenData, tokenSize)) {
        thiz->currentSection = iter;
        return true;
      }
    }
  }

  for (uint8_t index = thiz->sel; index < NQ_ARRAY_LENGTH(s_sections); index++) {
    const struct NQDBCParserSection* iter = &s_sections[index];
    if (isKeywordEqual(iter->keyword, tokenData, tokenSize)) {
      thiz->sel = index;
      thiz->currentSection = iter;
      thiz->parentSection = NULL;
      return true;
    }
  }

  // NQ_LOGW("Keyword '%.*s' disrupts the sequence of the document", (int)tokenSize, tokenData);

  for (size_t index = 0; index < thiz->sel; index++) {
    const struct NQDBCParserSection* iter = &s_sections[index];
    if (isKeywordEqual(iter->keyword, tokenData, tokenSize)) {
      thiz->currentSection = iter;
      thiz->parentSection = NULL;
      return true;
    }
  }

  return false;
}

static int NQDBCParser_next(NQDBCParser* thiz, const char** start, const char* end)
{
  int ret;
  bool repeat;
  uint32_t keywordMask;
  DBCToken token;
  NQDBCSection section;

  if (start != NULL)
    ret = DBCLexer_next(&thiz->lexer, start, end, &token);
  else {
    ret = DBCLexer_finish(&thiz->lexer, &token);
    // check lexer is empty
  }

  switch (ret) {
  case kLexerTokenDone:
    break;

  case kLexerWaitSymbol:
    return 0;

  case kLexerNumberError:
    return kNQDBCErrorTokenNumber;

  case kLexerStringError:
    return kNQDBCErrorTokenString;

  case kLexerBufferError:
    return kNQDBCErrorBufferOverflow;

  default:
    return kNQDBCErrorInternal;
  }

  do {
    if (!thiz->hasMachine) {
      if (token.type == kSymbolToken && token.data[0] == '\n') {
        NQ_ASSERT(token.size == 1);
        return 0;
      }

      if (token.type != kCIdentifierToken)
        return kNQDBCErrorSectionKeyword;

      if (!NQDBCParser_chooseSection(thiz, token.data, token.size)) {
        return kNQDBCErrorSectionKeyword;
      }

      keywordMask = (1 << thiz->currentSection->identifier);
      switch (thiz->currentSection->required) {
      case '?':
      case '1':
        if (thiz->keywordUseMap & keywordMask)
          return thiz->currentSection->errorCode;
        break;

      case '*':
        break;

      default:
        NQ_ASSERT(0);
        return thiz->currentSection->errorCode;
      }

      thiz->keywordUseMap |= keywordMask;
      thiz->hasMachine = true;
      DBCMachine_init(&thiz->machine, thiz->currentSection->entry);
      break;
    }

    if (DBCMachine_isOverflow(&thiz->machine))
      return kNQDBCErrorStackOverflow;

    if (!DBCMachine_next(&thiz->machine, &token, &repeat))
      return thiz->currentSection->errorCode;

    if (!DBCMachine_result(&thiz->machine, &section)) {
      if (start == NULL)
        return kNQDBCErrorSectionKeyword;
      return 0;
    }

    if (!DBCMachine_empty(&thiz->machine)) {
      NQ_ASSERT(0);
      return thiz->currentSection->errorCode;
    }

    if (section.type != (enum NQDBCSectionType)kNQDBCSectionEmpty && thiz->callback && !thiz->callback(thiz->userdata, &section))
      return kNQDBCErrorUser;

    if (!DBCLexer_empty(&thiz->lexer)) {
      NQ_ASSERT(0);
      return thiz->currentSection->errorCode;
    }

    if (thiz->parentSection == NULL) {
      thiz->parentSection = thiz->currentSection;
    }

    thiz->hasMachine = false;

    DBCLexer_reset(&thiz->lexer);
  } while (repeat);

  return 0;
}

static void NQDBCParser_init(NQDBCParser* thiz, void* userdata, NQDBCParserCallback* callback, void* allocator, NQDBCAllocFn* alloc, NQDBCFreeFn* free)
{
  thiz->userdata = userdata;
  thiz->callback = callback;

  thiz->allocator = allocator;
  thiz->alloc = alloc;
  thiz->free = free;

  thiz->errorCode = 0;

  thiz->line = 0;
  thiz->colume = 0;

  thiz->sel = 0;
  thiz->keywordUseMap = 0;

  thiz->parentSection = NULL;
  thiz->currentSection = &s_sections[thiz->sel];
  thiz->hasMachine = false;

  DBCLexer_init(&thiz->lexer, thiz->buffer, sizeof(thiz->buffer));
}

NQDBCParser* NQDBCParser_create(void* userdata, NQDBCParserCallback* callback)
{
  return NQDBCParser_create2(userdata, callback, NULL, &NQDBCAllocDefault, &NQDBCFreeDefault);
}

NQDBCParser* NQDBCParser_create2(void* userdata, NQDBCParserCallback* callback, void* allocator, NQDBCAllocFn* alloc, NQDBCFreeFn* free)
{
  if (alloc == NULL) {
    return NULL;
  }
  if (free == NULL) {
    free = &NQDBCFreeNope;
  }

  NQDBCParser* thiz = (NQDBCParser*)alloc(allocator, sizeof(struct NQDBCParser));
  if (thiz == NULL)
    return NULL;

  NQDBCParser_init(thiz, userdata, callback, allocator, alloc, free);

  return thiz;
}

void NQDBCParser_destroy(NQDBCParser* thiz)
{
  void* allocator = thiz->allocator;
  NQDBCFreeFn* freeFn = thiz->free;

  freeFn(allocator, thiz);
}

bool NQDBCParser_append(NQDBCParser* thiz, const char* data, size_t size)
{
  const char* start = data;
  const char* end = data + size;

  while (thiz->errorCode == 0 && start < end)
    thiz->errorCode = NQDBCParser_next(thiz, &start, end);

  return thiz->errorCode == 0;
}

bool NQDBCParser_finish(NQDBCParser* thiz)
{
  if (thiz->errorCode == 0)
    thiz->errorCode = NQDBCParser_next(thiz, NULL, NULL);

  return thiz->errorCode == 0;
}
