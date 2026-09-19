/*
 * MIT License
 *
 * Copyright (c) 2022-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_HTTP_HTTPMULTIPARTPARSER_H
#define _LIBNETQ_HTTP_HTTPMULTIPARTPARSER_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum NQHTTPMultiPartType {
  kNQHTTPMultiPartBegin,
  kNQHTTPMultiPartBoundary,
  kNQHTTPMultiPartFieldName,
  kNQHTTPMultiPartFieldValue,
  kNQHTTPMultiPartBodyPart,
  kNQHTTPMultiPartFinish,
} NQHTTPMultiPartType;

typedef bool (NQHTTPMultiPartParserCallback) (void* userdata, NQHTTPMultiPartType type, const char* data, size_t size);

typedef struct NQHTTPMultiPartParser NQHTTPMultiPartParser;
struct NQHTTPMultiPartParser {
  void* userdata;
  NQHTTPMultiPartParserCallback* callback;
  int state;
  uint8_t delimiterIndex;
  uint8_t delimiterLength;
  uint16_t bufferLength;
  uint32_t totalBodyPartBytes;
  char delimiter[80];
  char buffer[176];
};

NQ_EXPORT NQHTTPMultiPartParser* NQHTTPMultiPartParser_create(const char* boundary, NQHTTPMultiPartParserCallback callback, void* userdata);
NQ_EXPORT void NQHTTPMultiPartParser_destroy(NQHTTPMultiPartParser*);

NQ_EXPORT void NQHTTPMultiPartParser_init(NQHTTPMultiPartParser*, const char* boundary, NQHTTPMultiPartParserCallback callback, void* userdata);
static inline void NQHTTPMultiPartParser_finalize(NQHTTPMultiPartParser* thiz)
{
  NQ_UNUSED_PARAM(thiz);
}

NQ_EXPORT bool NQHTTPMultiPartParser_append(NQHTTPMultiPartParser*, const char* data, size_t size);
NQ_EXPORT bool NQHTTPMultiPartParser_finish(NQHTTPMultiPartParser*);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_HTTP_HTTPMULTIPARTPARSER_H */
