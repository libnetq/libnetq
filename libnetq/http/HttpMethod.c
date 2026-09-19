/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/http/HttpMethod.h"

#include <libnetq/string/StringUtil.h>

NQHttpMethod NQHttpMethodParse(const char* str)
{
  if (NQIsHttpGetMethod(str))
    return kNQHttpGetMethod;
  else if (NQIsHttpHeadMethod(str))
    return kNQHttpHeadMethod;
  else if (NQIsHttpPostMethod(str))
    return kNQHttpPostMethod;
  else if (NQIsHttpPutMethod(str))
    return kNQHttpPutMethod;
  else if (NQIsHttpDeleteMethod(str))
    return kNQHttpDeleteMethod;
  else if (NQIsHttpConnectMethod(str))
    return kNQHttpConnectMethod;
  else if (NQIsHttpOptionsMethod(str))
    return kNQHttpOptionsMethod;
  else if (NQIsHttpTraceMethod(str))
    return kNQHttpTraceMethod;
  else if (NQIsHttpPatchMethod(str))
    return kNQHttpPatchMethod;

  return kNQHttpUnknownMethod;
}

const char* NQHttpMethodToCStr(NQHttpMethod method)
{
  switch (method) {
  case kNQHttpGetMethod:
    return NQ_HTTP_GET;
  case kNQHttpHeadMethod:
    return NQ_HTTP_HEAD;
  case kNQHttpPostMethod:
    return NQ_HTTP_POST;
  case kNQHttpPutMethod:
    return NQ_HTTP_PUT;
  case kNQHttpDeleteMethod:
    return NQ_HTTP_DELETE;
  case kNQHttpConnectMethod:
    return NQ_HTTP_CONNECT;
  case kNQHttpOptionsMethod:
    return NQ_HTTP_OPTIONS;
  case kNQHttpTraceMethod:
    return NQ_HTTP_TRACE;
  case kNQHttpPatchMethod:
    return NQ_HTTP_PATCH;
  default:
    return NULL;
  }
}

bool NQIsHttpGetMethod(const char* method)
{
  return NQStrcmp(NQ_HTTP_GET, method) == 0;
}

bool NQIsHttpHeadMethod(const char* method)
{
  return NQStrcmp(NQ_HTTP_HEAD, method) == 0;
}

bool NQIsHttpPostMethod(const char* method)
{
  return NQStrcmp(NQ_HTTP_POST, method) == 0;
}

bool NQIsHttpPutMethod(const char* method)
{
  return NQStrcmp(NQ_HTTP_PUT, method) == 0;
}

bool NQIsHttpDeleteMethod(const char* method)
{
  return NQStrcmp(NQ_HTTP_DELETE, method) == 0;
}

bool NQIsHttpConnectMethod(const char* method)
{
  return NQStrcmp(NQ_HTTP_CONNECT, method) == 0;
}

bool NQIsHttpOptionsMethod(const char* method)
{
  return NQStrcmp(NQ_HTTP_OPTIONS, method) == 0;
}

bool NQIsHttpTraceMethod(const char* method)
{
  return NQStrcmp(NQ_HTTP_TRACE, method) == 0;
}

bool NQIsHttpPatchMethod(const char* method)
{
  return NQStrcmp(NQ_HTTP_PATCH, method) == 0;
}
