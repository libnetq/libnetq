/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_WEB_REQUEST_H
#define _LIBNETQ_WEB_REQUEST_H

#include <libnetq/VA.h>
#include <libnetq/UrlPath.h>
#include <libnetq/http/HttpMethod.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQWebServer NQWebServer;
typedef struct NQWebRequest NQWebRequest;
typedef struct NQWebResponse NQWebResponse;

typedef struct NQWebRequestClass NQWebRequestClass;
struct NQWebRequestClass {
  const char* (*getQuery)     (const NQWebRequest*, const char* name);
  const char* (*getCookie)    (const NQWebRequest*, const char* name);
  const char* (*getHeader)    (const NQWebRequest*, const char* header);
};

typedef struct NQWebRequestOperations NQWebRequestOperations;
struct NQWebRequestOperations {
  int    (*init)    (NQWebRequest*, void* data);
  size_t (*receive) (NQWebRequest*, const char* data, size_t size);
  int    (*handler) (NQWebRequest*, NQWebResponse*);
  void   (*release) (NQWebRequest*);
};

struct NQWebRequest {
  const NQWebRequestClass* clazz;

  const char* url;
  const char* method;
  const char* version;

  const NQWebRequestOperations* operations;

  NQUrlPath* urlPath;
  NQWebServer* server;
  void* userdata;
};

static inline void NQWebRequest_init(NQWebRequest* thiz)
{
  thiz->urlPath = NULL;
  thiz->operations = NULL;
}

static inline void NQWebRequest_finalize(NQWebRequest* thiz)
{
  if (thiz->urlPath) {
    NQUrlPath_destroy(thiz->urlPath);
  }
}

static inline NQWebServer* NQWebRequest_server(const NQWebRequest* thiz)
{
  return thiz->server;
}

static inline const char* NQWebRequest_url(const NQWebRequest* thiz)
{
  return thiz->url;
}

static inline const char* NQWebRequest_method(const NQWebRequest* thiz)
{
  return thiz->method;
}

static inline const char* NQWebRequest_version(const NQWebRequest* thiz)
{
  return thiz->version;
}

static inline bool NQWebRequest_isGET(const NQWebRequest* thiz)
{
  return NQIsHttpGetMethod(thiz->method);
}

static inline bool NQWebRequest_isPOST(const NQWebRequest* thiz)
{
  return NQIsHttpPostMethod(thiz->method);
}

static inline const char* NQWebRequest_getSegment(const NQWebRequest* thiz, const char* name)
{
  return NQUrlPath_segment(thiz->urlPath, name);
}

static inline const char* NQWebRequest_getQuery(const NQWebRequest* thiz, const char* name)
{
  return thiz->clazz->getQuery(thiz, name);
}

static inline const char* NQWebRequest_getCookie(const NQWebRequest* thiz, const char* name)
{
  return thiz->clazz->getCookie(thiz, name);
}

static inline const char* NQWebRequest_getHeader(const NQWebRequest* thiz, const char* header)
{
  return thiz->clazz->getHeader(thiz, header);
}

static inline void* NQWebRequest_userdata(const NQWebRequest* thiz)
{
  return thiz->userdata;
}

static inline void NQWebRequest_setUserdata(NQWebRequest* thiz, void* userdata)
{
  thiz->userdata = userdata;
}

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_WEB_REQUEST_H */
