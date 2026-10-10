/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_CLASS_NAME "NQJSONParser"
#define NQ_LOG_TAG NQ_CLASS_NAME

#include "config.h"
#include "libnetq/json/JSONParser.h"

#include <libnetq/CType.h>
#include <libnetq/PrimitiveType.h>
#include <libnetq/string/String.h>
#include <libnetq/string/StringUtil.h>

enum State {
  kValueState,        /* expecting a value */
  kArrayFirstState,   /* after '[': value or ']' */
  kObjectFirstState,  /* after '{': key or '}' */
  kObjectKeyState,    /* after ',' in object: key */
  kColonState,        /* after key: ':' */
  kAfterValueState,   /* after value: ',' or closing bracket */
  kStringState,
  kEscapeState,
  kUnicodeState,
  kNullState,
  kTrueState,
  kFalseState,
  kNumMinusState,     /* '-' */
  kNumZeroState,      /* leading '0' */
  kNumIntState,       /* integer digits */
  kNumDotState,       /* '.' */
  kNumFracState,      /* fraction digits */
  kNumExpState,       /* 'e' or 'E' */
  kNumExpSignState,   /* exponent sign */
  kNumExpDigitsState, /* exponent digits */
  kDoneState,
  kErrorState,
};

#define MAX_DEPTH (sizeof(((NQJSONParser*)0)->depth) * 8)
#define BUFFER_SIZE (sizeof(((NQJSONParser*)0)->buffer))

void NQJSONParser_init(NQJSONParser* thiz, NQJSONParserCallback callback, void* userdata)
{
  NQMemset(thiz, 0, sizeof(*thiz));
  thiz->state = kValueState;
  thiz->callback = callback;
  thiz->userdata = userdata;
}

void NQJSONParser_finalize(NQJSONParser* thiz)
{
  thiz->state = kErrorState;
}

static bool fail(NQJSONParser* thiz)
{
  thiz->state = kErrorState;
  return false;
}

static bool emit(NQJSONParser* thiz, enum NQJSONParserEventType type, unsigned flags, const char* characters, size_t length)
{
  NQJSONParserEvent event;

  if (thiz->callback == NULL)
    return true;

  event.type = type;
  event.flags = flags;
  event.length = (unsigned)length;
  event.characters = characters;
  if (!thiz->callback(thiz->userdata, &event))
    return fail(thiz);
  return true;
}

static bool isObject(const NQJSONParser* thiz)
{
  unsigned index = thiz->depthCount - 1;
  return (thiz->depth[index / 8] >> (index % 8)) & 1;
}

static void endValue(NQJSONParser* thiz)
{
  thiz->state = (thiz->depthCount != 0) ? kAfterValueState : kDoneState;
}

static bool beginContainer(NQJSONParser* thiz, bool object)
{
  unsigned index = thiz->depthCount;
  uint8_t mask = (uint8_t)(1u << (index % 8));

  if (index >= MAX_DEPTH)
    return fail(thiz);

  if (object)
    thiz->depth[index / 8] |= mask;
  else
    thiz->depth[index / 8] &= (uint8_t)~mask;
  thiz->depthCount++;
  thiz->state = object ? kObjectFirstState : kArrayFirstState;

  return emit(thiz, object ? kNQJSONParserObjectBegin : kNQJSONParserArrayBegin, NQ_JSONPARSER_FIN_FLAG, NULL, 0);
}

static bool endContainer(NQJSONParser* thiz, bool object)
{
  if (thiz->depthCount == 0 || isObject(thiz) != object)
    return fail(thiz);

  thiz->depthCount--;
  endValue(thiz);

  return emit(thiz, object ? kNQJSONParserObjectEnd : kNQJSONParserArrayEnd, NQ_JSONPARSER_FIN_FLAG, NULL, 0);
}

static void beginString(NQJSONParser* thiz, unsigned flags)
{
  thiz->state = kStringState;
  thiz->flags = (uint8_t)flags;
  thiz->length = 0;
  thiz->surrogate = 0;
}

static bool flushString(NQJSONParser* thiz)
{
  size_t length = thiz->length;

  if (length == 0)
    return true;

  thiz->length = 0;
  return emit(thiz, kNQJSONParserString, thiz->flags, thiz->buffer, length);
}

static bool endString(NQJSONParser* thiz)
{
  size_t length = thiz->length;
  unsigned flags = thiz->flags;

  thiz->length = 0;
  if (flags & NQ_JSONPARSER_KEY_FLAG)
    thiz->state = kColonState;
  else
    endValue(thiz);

  return emit(thiz, kNQJSONParserString, flags | NQ_JSONPARSER_FIN_FLAG, thiz->buffer, length);
}

/* Appends a run of unescaped characters, copying it into the buffer when it fits
 * or emitting it directly from the input otherwise. */
static bool appendStringRun(NQJSONParser* thiz, const char* run, size_t size, bool fin)
{
  if (thiz->length + size > BUFFER_SIZE && !flushString(thiz))
    return false;

  if (thiz->length + size <= BUFFER_SIZE && !(fin && thiz->length == 0)) {
    NQMemcpy(thiz->buffer + thiz->length, run, size);
    thiz->length += (uint8_t)size;
    return fin ? endString(thiz) : true;
  }

  if (fin) {
    unsigned flags = thiz->flags;
    if (flags & NQ_JSONPARSER_KEY_FLAG)
      thiz->state = kColonState;
    else
      endValue(thiz);
    return emit(thiz, kNQJSONParserString, flags | NQ_JSONPARSER_FIN_FLAG, run, size);
  }

  return emit(thiz, kNQJSONParserString, thiz->flags, run, size);
}

static bool appendStringChar(NQJSONParser* thiz, uint32_t ch)
{
  char* ptr;

  if (thiz->length + 4u > BUFFER_SIZE && !flushString(thiz))
    return false;

  ptr = thiz->buffer + thiz->length;
  if (ch < 0x80) {
    *ptr++ = (char)ch;
  }
  else if (ch < 0x800) {
    *ptr++ = (char)(0xC0 | (ch >> 6));
    *ptr++ = (char)(0x80 | (ch & 0x3F));
  }
  else if (ch < 0x10000) {
    *ptr++ = (char)(0xE0 | (ch >> 12));
    *ptr++ = (char)(0x80 | ((ch >> 6) & 0x3F));
    *ptr++ = (char)(0x80 | (ch & 0x3F));
  }
  else {
    *ptr++ = (char)(0xF0 | (ch >> 18));
    *ptr++ = (char)(0x80 | ((ch >> 12) & 0x3F));
    *ptr++ = (char)(0x80 | ((ch >> 6) & 0x3F));
    *ptr++ = (char)(0x80 | (ch & 0x3F));
  }
  thiz->length = (uint8_t)(ptr - thiz->buffer);

  return true;
}

static bool appendUnicode(NQJSONParser* thiz)
{
  uint32_t ch = thiz->unicode;

  thiz->state = kStringState;
  if (thiz->surrogate != 0) {
    if (ch < 0xDC00 || ch > 0xDFFF)
      return fail(thiz);
    ch = 0x10000 + (((uint32_t)thiz->surrogate - 0xD800) << 10) + (ch - 0xDC00);
    thiz->surrogate = 0;
  }
  else if (ch >= 0xD800 && ch <= 0xDBFF) {
    thiz->surrogate = (uint16_t)ch;
    return true;
  }
  else if (ch >= 0xDC00 && ch <= 0xDFFF) {
    return fail(thiz);
  }

  return appendStringChar(thiz, ch);
}

static const char* parseString(NQJSONParser* thiz, const char* ptr, const char* end)
{
  const char* run = ptr;

  if (thiz->surrogate != 0) {
    if (*ptr != '\\')
      return fail(thiz), NULL;
    thiz->state = kEscapeState;
    return ptr + 1;
  }

  while (ptr < end && *ptr != '"' && *ptr != '\\' && (uint8_t)*ptr >= 0x20)
    ptr++;

  if (ptr < end && *ptr == '"')
    return appendStringRun(thiz, run, ptr - run, true) ? ptr + 1 : NULL;

  if (ptr != run && !appendStringRun(thiz, run, ptr - run, false))
    return NULL;

  if (ptr == end)
    return ptr;

  if (*ptr != '\\')
    return fail(thiz), NULL;

  thiz->state = kEscapeState;
  return ptr + 1;
}

static bool parseEscape(NQJSONParser* thiz, char ch)
{
  if (thiz->surrogate != 0 && ch != 'u')
    return fail(thiz);

  thiz->state = kStringState;
  switch (ch) {
  case '"':
  case '\\':
  case '/':
    return appendStringChar(thiz, (uint8_t)ch);
  case 'b':
    return appendStringChar(thiz, '\b');
  case 'f':
    return appendStringChar(thiz, '\f');
  case 'n':
    return appendStringChar(thiz, '\n');
  case 'r':
    return appendStringChar(thiz, '\r');
  case 't':
    return appendStringChar(thiz, '\t');
  case 'u':
    thiz->state = kUnicodeState;
    thiz->unicode = 0;
    thiz->counter = 0;
    return true;
  default:
    return fail(thiz);
  }
}

static bool parseUnicode(NQJSONParser* thiz, char ch)
{
  if (!NQIsHexDigit(ch))
    return fail(thiz);

  thiz->unicode = (uint16_t)((thiz->unicode << 4) | NQToHexValue(ch));
  if (++thiz->counter < 4)
    return true;

  return appendUnicode(thiz);
}

static bool parseLiteral(NQJSONParser* thiz, char ch)
{
  const char* literal;
  size_t size;
  enum NQJSONParserEventType type;

  switch (thiz->state) {
  case kNullState:
    literal = NQ_NULL_STRING;
    size = NQ_CSTR_LENGTH(NQ_NULL_STRING);
    type = kNQJSONParserNull;
    break;
  case kTrueState:
    literal = NQ_TRUE_STRING;
    size = NQ_CSTR_LENGTH(NQ_TRUE_STRING);
    type = kNQJSONParserBool;
    break;
  default:
    literal = NQ_FALSE_STRING;
    size = NQ_CSTR_LENGTH(NQ_FALSE_STRING);
    type = kNQJSONParserBool;
    break;
  }

  if (literal[thiz->counter] != ch)
    return fail(thiz);
  if (++thiz->counter < size)
    return true;

  endValue(thiz);
  return emit(thiz, type, NQ_JSONPARSER_FIN_FLAG, literal, size);
}

static bool isNumberComplete(int state)
{
  return state == kNumZeroState || state == kNumIntState || state == kNumFracState || state == kNumExpDigitsState;
}

static bool endNumber(NQJSONParser* thiz)
{
  enum NQJSONParserEventType type;

  switch (thiz->state) {
  case kNumFracState:
    type = kNQJSONParserFraction;
    break;
  case kNumExpDigitsState:
    type = kNQJSONParserExponent;
    break;
  default:
    type = kNQJSONParserNumber;
    break;
  }

  thiz->buffer[thiz->length] = '\0';
  endValue(thiz);
  return emit(thiz, type, NQ_JSONPARSER_FIN_FLAG, thiz->buffer, thiz->length);
}

static int nextNumberState(int state, char ch)
{
  bool digit = NQIsDigit(ch);
  bool exponent = (ch == 'e' || ch == 'E');

  switch (state) {
  case kNumMinusState:
    if (ch == '0')
      return kNumZeroState;
    return digit ? kNumIntState : kErrorState;
  case kNumZeroState:
    if (ch == '.')
      return kNumDotState;
    return exponent ? kNumExpState : kDoneState;
  case kNumIntState:
    if (digit)
      return kNumIntState;
    if (ch == '.')
      return kNumDotState;
    return exponent ? kNumExpState : kDoneState;
  case kNumDotState:
    return digit ? kNumFracState : kErrorState;
  case kNumFracState:
    if (digit)
      return kNumFracState;
    return exponent ? kNumExpState : kDoneState;
  case kNumExpState:
    if (ch == '+' || ch == '-')
      return kNumExpSignState;
    return digit ? kNumExpDigitsState : kErrorState;
  case kNumExpSignState:
    return digit ? kNumExpDigitsState : kErrorState;
  case kNumExpDigitsState:
    return digit ? kNumExpDigitsState : kDoneState;
  default:
    return kErrorState;
  }
}

/* Returns true if the character was consumed as part of the number. */
static bool parseNumber(NQJSONParser* thiz, char ch, bool* consumed)
{
  int next = nextNumberState(thiz->state, ch);

  *consumed = false;
  if (next == kErrorState)
    return fail(thiz);
  if (next == kDoneState)
    return endNumber(thiz);

  /* keep room for the terminating NUL */
  if (thiz->length + 1u >= BUFFER_SIZE)
    return fail(thiz);

  thiz->buffer[thiz->length++] = ch;
  thiz->state = next;
  *consumed = true;
  return true;
}

static bool beginValue(NQJSONParser* thiz, char ch)
{
  switch (ch) {
  case '{':
    return beginContainer(thiz, true);
  case '[':
    return beginContainer(thiz, false);
  case '"':
    beginString(thiz, 0);
    return true;
  case 'n':
    thiz->state = kNullState;
    thiz->counter = 1;
    return true;
  case 't':
    thiz->state = kTrueState;
    thiz->counter = 1;
    return true;
  case 'f':
    thiz->state = kFalseState;
    thiz->counter = 1;
    return true;
  case '-':
    thiz->state = kNumMinusState;
    break;
  case '0':
    thiz->state = kNumZeroState;
    break;
  default:
    if (!NQIsDigit(ch))
      return fail(thiz);
    thiz->state = kNumIntState;
    break;
  }

  thiz->buffer[0] = ch;
  thiz->length = 1;
  return true;
}

bool NQJSONParser_append(NQJSONParser* thiz, const char* data, size_t size)
{
  const char* ptr = data;
  const char* end = data + size;

  while (ptr < end) {
    char ch = *ptr;
    bool ok;

    switch (thiz->state) {
    case kStringState:
      ptr = parseString(thiz, ptr, end);
      if (ptr == NULL)
        return false;
      continue;

    case kEscapeState:
      ok = parseEscape(thiz, ch);
      break;

    case kUnicodeState:
      ok = parseUnicode(thiz, ch);
      break;

    case kNullState:
    case kTrueState:
    case kFalseState:
      ok = parseLiteral(thiz, ch);
      break;

    case kNumMinusState:
    case kNumZeroState:
    case kNumIntState:
    case kNumDotState:
    case kNumFracState:
    case kNumExpState:
    case kNumExpSignState:
    case kNumExpDigitsState: {
      bool consumed;
      if (!parseNumber(thiz, ch, &consumed))
        return false;
      if (!consumed)
        continue; /* reprocess the terminating character */
      ok = true;
      break;
    }

    case kErrorState:
      return false;

    default:
      if (NQIsSpace(ch)) {
        ok = true;
        break;
      }

      switch (thiz->state) {
      case kValueState:
        ok = beginValue(thiz, ch);
        break;
      case kArrayFirstState:
        ok = (ch == ']') ? endContainer(thiz, false) : beginValue(thiz, ch);
        break;
      case kObjectFirstState:
        if (ch == '}') {
          ok = endContainer(thiz, true);
          break;
        }
        /* fall through */
      case kObjectKeyState:
        if (ch != '"')
          return fail(thiz);
        beginString(thiz, NQ_JSONPARSER_KEY_FLAG);
        ok = true;
        break;
      case kColonState:
        if (ch != ':')
          return fail(thiz);
        thiz->state = kValueState;
        ok = true;
        break;
      case kAfterValueState:
        if (ch == ',') {
          thiz->state = isObject(thiz) ? kObjectKeyState : kValueState;
          ok = true;
        }
        else if (ch == ']') {
          ok = endContainer(thiz, false);
        }
        else if (ch == '}') {
          ok = endContainer(thiz, true);
        }
        else {
          return fail(thiz);
        }
        break;
      default: /* kDoneState: only trailing whitespace is allowed */
        return fail(thiz);
      }
      break;
    }

    if (!ok)
      return false;
    ptr++;
  }

  return true;
}

bool NQJSONParser_finish(NQJSONParser* thiz)
{
  /* a top-level number is terminated by the end of input */
  if (thiz->depthCount == 0 && isNumberComplete(thiz->state) && !endNumber(thiz))
    return false;

  return (thiz->state == kDoneState);
}
