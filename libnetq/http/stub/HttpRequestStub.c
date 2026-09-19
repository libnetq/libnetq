/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/http/HttpRequest.h"

#ifdef NQCONFIG_USE_STUB_HTTPREQUESTSYNC

#include <libnetq/ErrorCode.h>

NQHttpRequest* NQHttpRequest_create(NQHttpRequestWriteCallback callback, void* userdata)
{
  NQ_UNUSED_PARAM(callback);
  NQ_UNUSED_PARAM(userdata);
  return NULL;
}

void NQHttpRequest_release(NQHttpRequest* thiz)
{
  NQ_UNUSED_PARAM(thiz);
}

int NQHttpRequest_performSync(NQHttpRequest* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return -NQ_ENOTSUPP;
}

const char* NQHttpRequest_lastErrorMessage(NQHttpRequest* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return NULL;
}

bool NQHttpRequest_setUrl(NQHttpRequest* thiz, const char* url)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(url);
  return false;
}

bool NQHttpRequest_setMethod(NQHttpRequest* thiz, const char* method)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(method);
  return false;
}

bool NQHttpRequest_setPostData(NQHttpRequest* thiz, const void* data, size_t size)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(data);
  NQ_UNUSED_PARAM(size);
  return false;
}

bool NQHttpRequest_setFollowLocation(NQHttpRequest* thiz, bool value)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(value);
  return false;
}

bool NQHttpRequest_setTimeoutMs(NQHttpRequest* thiz, int64_t timeoutMs)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(timeoutMs);
  return false;
}

bool NQHttpRequest_addHeader(NQHttpRequest* thiz, const char* name, const char* value)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(name);
  NQ_UNUSED_PARAM(value);
  return false;
}

int NQHttpRequest_responseStatusCode(NQHttpRequest* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return 0;
}

const char* NQHttpRequest_responseReasonText(NQHttpRequest* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return NULL;
}

bool NQHttpRequest_responseTimeMs(NQHttpRequest* thiz, int64_t* result)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(result);
  return false;
}

NQHttpRequestHeaderIter* NQHttpRequest_responseHeaderFirst(NQHttpRequest* thiz)
{
  NQ_UNUSED_PARAM(thiz);
  return NULL;
}

NQHttpRequestHeaderIter* NQHttpRequest_responseHeaderNext(NQHttpRequest* thiz, NQHttpRequestHeaderIter* iter)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(iter);
  return NULL;
}

const char* NQHttpRequestHeaderIter_name(NQHttpRequestHeaderIter* iter)
{
  NQ_UNUSED_PARAM(iter);
  return NULL;
}

const char* NQHttpRequestHeaderIter_value(NQHttpRequestHeaderIter* iter)
{
  NQ_UNUSED_PARAM(iter);
  return NULL;
}

#endif
