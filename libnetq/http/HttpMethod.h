/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_HTTP_HTTPMETHOD_H
#define _LIBNETQ_HTTP_HTTPMETHOD_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

enum NQHttpMethod {
  kNQHttpUnknownMethod,
  kNQHttpGetMethod,
  kNQHttpHeadMethod,
  kNQHttpPostMethod,
  kNQHttpPutMethod,
  kNQHttpDeleteMethod,
  kNQHttpConnectMethod,
  kNQHttpOptionsMethod,
  kNQHttpTraceMethod,
  kNQHttpPatchMethod,
};

typedef enum NQHttpMethod NQHttpMethod;

#define NQ_HTTP_GET     "GET"
#define NQ_HTTP_HEAD    "HEAD"
#define NQ_HTTP_POST    "POST"
#define NQ_HTTP_PUT     "PUT"
#define NQ_HTTP_DELETE  "DELETE"
#define NQ_HTTP_CONNECT "CONNECT"
#define NQ_HTTP_OPTIONS "OPTIONS"
#define NQ_HTTP_TRACE   "TRACE"
#define NQ_HTTP_PATCH   "PATCH"

NQ_EXPORT NQHttpMethod NQHttpMethodParse(const char* str);
NQ_EXPORT const char* NQHttpMethodToCStr(NQHttpMethod);

NQ_EXPORT bool NQIsHttpGetMethod(const char* method);
NQ_EXPORT bool NQIsHttpHeadMethod(const char* method);
NQ_EXPORT bool NQIsHttpPostMethod(const char* method);
NQ_EXPORT bool NQIsHttpPutMethod(const char* method);
NQ_EXPORT bool NQIsHttpDeleteMethod(const char* method);
NQ_EXPORT bool NQIsHttpConnectMethod(const char* method);
NQ_EXPORT bool NQIsHttpOptionsMethod(const char* method);
NQ_EXPORT bool NQIsHttpTraceMethod(const char* method);
NQ_EXPORT bool NQIsHttpPatchMethod(const char* method);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_HTTP_HTTPMETHOD_H */
