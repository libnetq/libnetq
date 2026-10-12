/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_CLASS_NAME "NQHttpRequest"
#define NQ_LOG_TAG NQ_CLASS_NAME

#include "config.h"
#include "libnetq/http/HttpRequest.h"

#ifdef HTTPREQUEST_BACKEND_STUB

#include <libnetq/ErrorCode.h>

const char* NQHttpRequestBackendVersion(void)
{
  return NULL;
}

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

NQHttpRequestError NQHttpRequest_lastError(NQHttpRequest* thiz)
{
  NQ_UNUSED_PARAM(thiz);

  return NQ_HTTP_REQUEST_ERROR_OTHER;
}

int NQHttpRequest_lastErrorCode(NQHttpRequest* thiz)
{
  NQ_UNUSED_PARAM(thiz);

  return -NQ_ENOTSUPP;
}

bool NQHttpRequest_setHeaderCallback(NQHttpRequest* thiz, NQHttpRequestHeaderCallback callback, void* userdata)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(callback);
  NQ_UNUSED_PARAM(userdata);

  return false;
}

bool NQHttpRequest_setDebugCallback(NQHttpRequest* thiz, NQHttpRequestDebugCallback callback, void* userdata)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(callback);
  NQ_UNUSED_PARAM(userdata);

  return false;
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

bool NQHttpRequest_setCustomMethod(NQHttpRequest* thiz, const char* method)
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

bool NQHttpRequest_setMaxRedirects(NQHttpRequest* thiz, long count)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(count);

  return false;
}

bool NQHttpRequest_setUploadData(NQHttpRequest* thiz, const void* data, size_t size)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(data);
  NQ_UNUSED_PARAM(size);

  return false;
}

bool NQHttpRequest_addFormPart(NQHttpRequest* thiz, const char* name, const void* data, size_t size,
                               const char* filename, const char* contentType)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(name);
  NQ_UNUSED_PARAM(data);
  NQ_UNUSED_PARAM(size);
  NQ_UNUSED_PARAM(filename);
  NQ_UNUSED_PARAM(contentType);

  return false;
}

bool NQHttpRequest_addFormFile(NQHttpRequest* thiz, const char* name, const char* path,
                               const char* filename, const char* contentType)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(name);
  NQ_UNUSED_PARAM(path);
  NQ_UNUSED_PARAM(filename);
  NQ_UNUSED_PARAM(contentType);

  return false;
}

bool NQHttpRequest_setRange(NQHttpRequest* thiz, const char* range)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(range);

  return false;
}

bool NQHttpRequest_setResumeFrom(NQHttpRequest* thiz, int64_t offset)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(offset);

  return false;
}

bool NQHttpRequest_setAcceptEncoding(NQHttpRequest* thiz, const char* encodings)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(encodings);

  return false;
}

bool NQHttpRequest_setProxy(NQHttpRequest* thiz, const char* proxy)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(proxy);

  return false;
}

bool NQHttpRequest_setCookie(NQHttpRequest* thiz, const char* cookies)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(cookies);

  return false;
}

bool NQHttpRequest_setCookieFile(NQHttpRequest* thiz, const char* path)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(path);

  return false;
}

bool NQHttpRequest_setCookieJar(NQHttpRequest* thiz, const char* path)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(path);

  return false;
}

bool NQHttpRequest_setVerifyingPeerSsl(NQHttpRequest* thiz, bool value)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(value);

  return false;
}

bool NQHttpRequest_setVerifyingHostSsl(NQHttpRequest* thiz, bool value)
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

bool NQHttpRequest_setCredentials(NQHttpRequest* thiz, const char* username, const char* password)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(username);
  NQ_UNUSED_PARAM(password);

  return false;
}

bool NQHttpRequest_setAuthMethods(NQHttpRequest* thiz, unsigned methods)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(methods);

  return false;
}

bool NQHttpRequest_setConnectTimeoutMs(NQHttpRequest* thiz, int64_t timeoutMs)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(timeoutMs);

  return false;
}

bool NQHttpRequest_setIdleTimeoutMs(NQHttpRequest* thiz, int64_t timeoutMs)
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

bool NQHttpRequest_addHeaderLine(NQHttpRequest* thiz, const char* line)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(line);

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

const char* NQHttpRequest_responseVersion(NQHttpRequest* thiz)
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

const char* NQHttpRequest_redirectUrl(NQHttpRequest* thiz)
{
  NQ_UNUSED_PARAM(thiz);

  return NULL;
}

const char* NQHttpRequest_remoteAddress(NQHttpRequest* thiz)
{
  NQ_UNUSED_PARAM(thiz);

  return NULL;
}

int NQHttpRequest_remotePort(NQHttpRequest* thiz)
{
  NQ_UNUSED_PARAM(thiz);

  return -1;
}

const char* NQHttpRequest_effectiveUrl(NQHttpRequest* thiz)
{
  NQ_UNUSED_PARAM(thiz);

  return NULL;
}

long NQHttpRequest_redirectCount(NQHttpRequest* thiz)
{
  NQ_UNUSED_PARAM(thiz);

  return 0;
}

bool NQHttpRequest_timings(NQHttpRequest* thiz, NQHttpRequestTimings* result)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(result);

  return false;
}

size_t NQHttpRequest_responseHeaderCount(NQHttpRequest* thiz)
{
  NQ_UNUSED_PARAM(thiz);

  return 0;
}

const char* NQHttpRequest_responseHeaderNameAt(NQHttpRequest* thiz, size_t index)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(index);

  return NULL;
}

const char* NQHttpRequest_responseHeaderValueAt(NQHttpRequest* thiz, size_t index)
{
  NQ_UNUSED_PARAM(thiz);
  NQ_UNUSED_PARAM(index);

  return NULL;
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

int NQSendEmailSync(struct NQSendEmailParams* params)
{
  NQ_UNUSED_PARAM(params);

  return -NQ_ENOTSUPP;
}

#endif
