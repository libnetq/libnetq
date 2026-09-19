/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_JSON_JSONPARSER_H
#define _LIBNETQ_JSON_JSONPARSER_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

enum NQJSONParserEventType {
  kNQJSONParserNull,
  kNQJSONParserBool,
  kNQJSONParserString,
  kNQJSONParserNumber,
  kNQJSONParserFraction,
  kNQJSONParserExponent,
  kNQJSONParserArrayBegin,
  kNQJSONParserArrayEnd,
  kNQJSONParserObjectBegin,
  kNQJSONParserObjectEnd,
};

#define NQ_JSONPARSER_KEY_FLAG 1
#define NQ_JSONPARSER_FIN_FLAG 2

typedef struct NQJSONParserEvent NQJSONParserEvent;
struct NQJSONParserEvent {
  enum NQJSONParserEventType type;
  unsigned flags;
  unsigned length;
  const char* characters;
};

typedef bool (*NQJSONParserCallback) (void* userdata, const NQJSONParserEvent* event);

typedef struct NQJSONParser NQJSONParser;
struct NQJSONParser {
  int state;

  NQJSONParserCallback callback;
  void* userdata;

  uint16_t unicode;
  uint16_t surrogate;
  uint8_t flags;
  uint8_t counter;
  uint8_t length;
  uint8_t depthCount;
  uint8_t depth[15];
  char buffer[48];
};

NQ_EXPORT void NQJSONParser_init(NQJSONParser*, NQJSONParserCallback callback, void* userdata);
NQ_EXPORT void NQJSONParser_finalize(NQJSONParser*);
NQ_EXPORT bool NQJSONParser_append(NQJSONParser*, const char* data, size_t size);
NQ_EXPORT bool NQJSONParser_finish(NQJSONParser*);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_JSON_JSONPARSER_H */
