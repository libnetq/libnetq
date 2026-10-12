/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_WEB_WEBRESPONSE_H
#define _LIBNETQ_WEB_WEBRESPONSE_H

#include <libnetq/VA.h>
#include <libnetq/List.h>
#include <libnetq/string/StringPrint.h>
#include <libnetq/web/WebCookie.h>
#include <libnetq/web/WebWriter.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQWebServer NQWebServer;
typedef struct NQWebRequest NQWebRequest;
typedef struct NQWebResponse NQWebResponse;

struct NQWebResponseOperations {
  bool (*addHeader) (NQWebResponse*, const char* header, const char* value);
  int (*write) (NQWebResponse*, const void* data, size_t size);
  int (*flush) (NQWebResponse*);
};

struct NQWebResponse {
  const struct NQWebResponseOperations* operations;
  NQWebServer* server;
  NQWebRequest* request;
  NQStringPrint printfBuffer;
  struct NQWebWriter* firstWriter;
  NQWebWriter lastWriter;
};

static inline NQWebServer* NQWebResponse_server(const NQWebResponse* thiz)
{
  return thiz->server;
}

NQ_EXPORT void NQWebResponse_init(NQWebResponse*, const struct NQWebResponseOperations* operations, NQWebRequest* request);
NQ_EXPORT void NQWebResponse_finalize(NQWebResponse*);

NQ_EXPORT int NQWebResponse_printf(NQWebResponse*, const char* fmt, ...) NQ_ATTRIBUTE_PRINTF(2, 3);
NQ_EXPORT int NQWebResponse_vprintf(NQWebResponse*, const char* fmt, va_list args) NQ_ATTRIBUTE_PRINTF(2, 0);
NQ_EXPORT int NQWebResponse_write(NQWebResponse*, const void* data, size_t size);
NQ_EXPORT int NQWebResponse_flush(NQWebResponse*);

NQ_EXPORT bool NQWebResponse_setHeader(NQWebResponse*, const char* header, const char* value);
NQ_EXPORT bool NQWebResponse_addHeader(NQWebResponse*, const char* header, const char* value);
NQ_EXPORT bool NQWebResponse_addCookie(NQWebResponse*, const NQWebCookie* cookie);
NQ_EXPORT bool NQWebResponse_deleteCookie(NQWebResponse*, const char* name, const char* path, const char* domain);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_WEB_WEBRESPONSE_H */
