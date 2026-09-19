/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/web/WebRestArchApi.h"

#include <libnetq/http/HttpHeader.h>
#include <libnetq/string/String.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/json/JSONWriter.h>
#include <libnetq/http/MediaType.h>
#include <libnetq/CPU.h>
#include <libnetq/Event.h>
#include <libnetq/web/WebRequest.h>
#include <libnetq/web/WebResponse.h>

#ifdef NQ_CPU_BIG_ENDIAN
#define ENDIANNESS "BE"
#else
#define ENDIANNESS "LE"
#endif

#ifdef NQ_CPU_64BIT
#define NBITS "64"
#else
#define NBITS "32"
#endif

static const char kArchApiString[] =
"{"
  "\"cpu\":\"" NQ_CPU_NAME "\","
  "\"abi\":\"" NQ_CPU_ABI "\","
  "\"endianness\":\"" ENDIANNESS "\","
  "\"bits\":" NBITS
"}";

static NQWebBlob kArchApiBlob = {
  .type = NQ_MEDIATYPE_APPLICATION_JSON,
  .data = kArchApiString,
  .size = NQ_CSTR_LENGTH(kArchApiString),
};

NQWebRestArchApi* NQWebRestArchApiCreate(NQWebServer* server, struct NQWebRestArchParams* params)
{
  return (NQWebRestArchApi*)NQWebServer_createRequestBlob(server, NQ_HTTP_GET, params->url, &kArchApiBlob);
}

void NQWebRestArchApiDestroy(NQWebServer* server, NQWebRestArchApi* restApi)
{
  NQWebServer_destroyExecutor(server, (NQWebExecutor*)restApi);
}
