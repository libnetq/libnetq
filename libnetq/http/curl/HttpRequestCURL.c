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

#ifdef NQCONFIG_USE_CURL_HTTPREQUESTSYNC

#include <libnetq/http/curl/CURLAdapter.h>
#include <libnetq/http/curl/CURLHeaders.h>
#include <libnetq/http/HttpMethod.h>
#include <libnetq/http/HttpHeader.h>
#include <libnetq/MinMax.h>
#include <libnetq/Limits.h>
#include <libnetq/Log.h>
#include <libnetq/string/Strtox.h>
#include <libnetq/string/String.h>
#include <libnetq/string/StringData.h>
#include <libnetq/string/StringRange.h>
#include <libnetq/Malloc.h>
#include <libnetq/List.h>
#include <libnetq/Mutex.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/Assert.h>

enum {
  InitState,
  StatusState,
  HeaderState,
  DoneState,
  ErrorState,
};

struct NQHttpRequest {
  CURL* curl;
  NQCURLHeaders headers;
  NQHttpRequestWriteCallback callback;
  void* userdata;
  char errorBuffer[CURL_ERROR_SIZE];
  int state;
  int statucCode;
  NQStringData statusReason;
  NQListHead responseHeaders;
};

struct NQHttpRequestHeaderIter {
  NQListHead list;
  char* name;
  char value[1];
};

static NQ_MUTEX_DEFINE(g_mutex);
static unsigned g_initCounter = 0;

static void curlGlobalInit()
{
  NQMutex_lock(&g_mutex);
  if (g_initCounter++ == 0)
    curl_global_init(CURL_GLOBAL_ALL);
  NQMutex_unlock(&g_mutex);
}

static void curlGlobalCleanup()
{
  NQMutex_lock(&g_mutex);
  if (--g_initCounter == 0)
    curl_global_cleanup();
  NQMutex_unlock(&g_mutex);
}

static bool parseStatusLine(NQHttpRequest* thiz, const char* data, size_t size)
{
  NQHttpStatusLine statusLine;
  if (!NQHttpStatusLineParse(data, size, &statusLine))
    return false;

  char* end;
  unsigned long num = NQSimpleStrtoul(statusLine.code.characters, &end, 10);
  if (statusLine.code.characters + statusLine.code.length != end || num > NQ_INT_MAX)
    return false;

  thiz->statucCode = (int)num;
  return NQStringData_set2(&thiz->statusReason, statusLine.reason.characters, statusLine.reason.length);
}

static bool parseHeaderLine(NQHttpRequest* thiz, const char* data, size_t size)
{
  NQHttpHeaderLine headerLine;
  if (!NQHttpHeaderLineParse(data, size, &headerLine))
    return false;

  NQHttpRequestHeaderIter* hdrIter;
  size_t sizeInBytes = sizeof(*hdrIter) + headerLine.name.length + headerLine.value.length + 1;
  hdrIter = (NQHttpRequestHeaderIter*)NQMalloc(sizeInBytes);
  if (hdrIter == NULL)
    return false;

  hdrIter->name = (char*)hdrIter + sizeof(*hdrIter) + headerLine.value.length;
  memcpy(hdrIter->name, headerLine.name.characters, headerLine.name.length);
  hdrIter->name[headerLine.name.length] = '\0';
  memcpy(hdrIter->value, headerLine.value.characters, headerLine.value.length);
  hdrIter->value[headerLine.value.length] = '\0';
  NQ_ASSERT(hdrIter->name + headerLine.name.length + 1 == (char*)hdrIter + sizeInBytes);
  NQListHead_addBack(&thiz->responseHeaders, &hdrIter->list);

  return true;
}

static size_t headerCallback(char* str, size_t size, size_t n, void* userdata)
{
  NQHttpRequest* thiz = (NQHttpRequest*)userdata;
  size_t len = size * n;

  size_t sz = len;
  if (sz >= NQ_CSTR_LENGTH(NQ_HTTP_CRLF) && NQStrcmp(str + sz - NQ_CSTR_LENGTH(NQ_HTTP_CRLF), NQ_HTTP_CRLF) == 0)
    sz -= NQ_CSTR_LENGTH(NQ_HTTP_CRLF);

  if (thiz->state == StatusState) {
    if (!parseStatusLine(thiz, str, sz))
      thiz->state = ErrorState;
    else
      thiz->state = HeaderState;
  }
  else if (thiz->state == HeaderState) {
    if (sz == 0)
      thiz->state = DoneState;
    else if (!parseHeaderLine(thiz, str, sz))
      thiz->state = ErrorState;
  }

  return len;
}

static size_t writeCallback(char* str, size_t size, size_t n, void* userdata)
{
  NQHttpRequest* thiz = (NQHttpRequest*)userdata;
  size_t len = size * n;
  int ret = thiz->callback(thiz->userdata, str, len);
  NQ_UNUSED_PARAM(ret);
  return len;
}

static int toErrorCode(CURLcode res)
{
  if (res != CURLE_OK)
    return -NQ_EIO;
  return 0;
}

static inline int toBoolean(CURLcode res)
{
  return (res != CURLE_OK) ? false : true;
}

NQHttpRequest* NQHttpRequest_create(NQHttpRequestWriteCallback callback, void* userdata)
{
  curlGlobalInit();

  CURL* curl = curl_easy_init();
  if (curl == NULL) {
    curlGlobalCleanup();
    return NULL;
  }

  NQHttpRequest* thiz = (NQHttpRequest*)NQMalloc(sizeof(struct NQHttpRequest));
  if (thiz == NULL) {
    curlGlobalCleanup();
    return NULL;
  }

  thiz->curl = curl;
  NQCURLHeaders_init(&thiz->headers);

  thiz->callback = callback;
  thiz->userdata = userdata;

  thiz->errorBuffer[0] = '\0';

  curl_easy_setopt(thiz->curl, CURLOPT_WRITEFUNCTION, writeCallback);
  curl_easy_setopt(thiz->curl, CURLOPT_WRITEDATA, thiz);

  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, thiz);

  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, thiz->errorBuffer);

  thiz->state = InitState;
  thiz->statucCode = -1;
  NQStringData_init(&thiz->statusReason);
  NQListHead_init(&thiz->responseHeaders);

  return thiz;
}

void NQHttpRequest_release(NQHttpRequest* thiz)
{
  while (!NQListHead_isEmpty(&thiz->responseHeaders)) {
    NQHttpRequestHeaderIter* iter = NQ_CONTAINER_OF(thiz->responseHeaders.prev, struct NQHttpRequestHeaderIter, list);
    NQListHead_remove(&iter->list);
    NQFree(iter);
  }

  NQStringData_finalize(&thiz->statusReason);
  NQCURLHeaders_finalize(&thiz->headers);
  curl_easy_cleanup(thiz->curl);
  NQFree(thiz);
  curlGlobalCleanup();
}

int NQHttpRequest_performSync(NQHttpRequest* thiz)
{
  if (thiz->state == InitState)
    thiz->state = StatusState;
  return toErrorCode(curl_easy_perform(thiz->curl));
}

const char* NQHttpRequest_lastErrorMessage(NQHttpRequest* thiz)
{
  return thiz->errorBuffer;
}

bool NQHttpRequest_setUrl(NQHttpRequest* thiz, const char* url)
{
  return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_URL, url));
}

bool NQHttpRequest_setMethod(NQHttpRequest* thiz, const char* method)
{
  CURLcode res;

  if (!NQStrcmp(method, NQ_HTTP_GET))
    res = curl_easy_setopt(thiz->curl, CURLOPT_HTTPGET, 1L);
  else if (!NQStrcmp(method, NQ_HTTP_POST))
    res = curl_easy_setopt(thiz->curl, CURLOPT_POST, 1L);
  else if (!NQStrcmp(method, NQ_HTTP_HEAD))
    res = curl_easy_setopt(thiz->curl, CURLOPT_NOBODY, 1L);
  else
    res = curl_easy_setopt(thiz->curl, CURLOPT_CUSTOMREQUEST, method);

  return toBoolean(res);
}

bool NQHttpRequest_setPostData(NQHttpRequest* thiz, const void* data, size_t size)
{
  if (!toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_POSTFIELDS, data)))
    return false;
  if (!toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_POSTFIELDSIZE, size)))
    return false;
  return true;
}

bool NQHttpRequest_setFollowLocation(NQHttpRequest* thiz, bool value)
{
  return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_FOLLOWLOCATION, value ? 1L : 0L));
}

bool NQHttpRequest_setVerifyingPeerSsl(NQHttpRequest* thiz, bool value)
{
  return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_SSL_VERIFYPEER, value ? 1L : 0L));
}

bool NQHttpRequest_setVerifyingHostSsl(NQHttpRequest* thiz, bool value)
{
  return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_SSL_VERIFYHOST, value ? 2L : 0L));
}

bool NQHttpRequest_setTimeoutMs(NQHttpRequest* thiz, int64_t timeoutMs)
{
  timeoutMs = NQGetClamp(timeoutMs, NQ_LONG_MIN, NQ_LONG_MAX);
  return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_TIMEOUT_MS, (long)timeoutMs));
}

bool NQHttpRequest_addHeader(NQHttpRequest* thiz, const char* name, const char* value)
{
  return NQCURLHeaders_add(&thiz->headers, name, value);
}

int NQHttpRequest_responseStatusCode(NQHttpRequest* thiz)
{
  return thiz->statucCode;
}

const char* NQHttpRequest_responseReasonText(NQHttpRequest* thiz)
{
  return NQStringData_characters(&thiz->statusReason);
}

bool NQHttpRequest_responseTimeMs(NQHttpRequest* thiz, int64_t* result)
{
  curl_off_t totalTimeUs = 0;
  if (!toBoolean(curl_easy_getinfo(thiz->curl, CURLINFO_TOTAL_TIME_T, &totalTimeUs)))
      return false;
  *result = (int64_t)(totalTimeUs / 1000);
  return true;
}

NQHttpRequestHeaderIter* NQHttpRequest_responseHeaderFirst(NQHttpRequest* thiz)
{
  NQListHead* it = thiz->responseHeaders.next;
  if (it == &thiz->responseHeaders)
    return NULL;
  return NQ_CONTAINER_OF(it, struct NQHttpRequestHeaderIter, list);
}

NQHttpRequestHeaderIter* NQHttpRequest_responseHeaderNext(NQHttpRequest* thiz, NQHttpRequestHeaderIter* iter)
{
  NQListHead* it = iter->list.next;
  if (it == &thiz->responseHeaders)
    return NULL;
  return NQ_CONTAINER_OF(it, struct NQHttpRequestHeaderIter, list);
}

const char* NQHttpRequestHeaderIter_name(NQHttpRequestHeaderIter* iter)
{
  return iter->name;
}

const char* NQHttpRequestHeaderIter_value(NQHttpRequestHeaderIter* iter)
{
  return iter->value;
}

struct EmailUploadStatus {
  const char** sequence;
  const char* characters;
  size_t length;
};

static size_t emailPayloadSource(char* ptr, size_t size, size_t nmemb, void* userdata) {
  struct EmailUploadStatus* uploadCtx = (struct EmailUploadStatus*)userdata;

  size_t bufferSize = size * nmemb;
  if (bufferSize == 0)
      return 0;

  while (uploadCtx->length == 0) {
    if (*uploadCtx->sequence == NULL)
      return 0;
    uploadCtx->characters = *(uploadCtx->sequence++);
    uploadCtx->length = NQStrlen(uploadCtx->characters);
  }

  size_t n = NQGetMin(uploadCtx->length, bufferSize);
  memcpy(ptr, uploadCtx->characters, n);
  uploadCtx->characters += n;
  uploadCtx->length -= n;

  return n;
}

int NQSendEmailSync(struct NQSendEmailParams* params)
{
  if (params->smtp == NULL || params->username == NULL || params->password == NULL
      || params->from == NULL || params->to == NULL || params->subject == NULL
      || params->content == NULL) {
    return -NQ_EINVAL;
  }

  const char* sequence[] = {
    "To: ", params->to, NQ_HTTP_CRLF,
    "From: ", params->from, NQ_HTTP_CRLF,
    "Subject: ", params->subject, NQ_HTTP_CRLF,
    NQ_HTTP_CRLF,
    params->content,
    NQ_HTTP_CRLF,
    NULL,
  };

  struct EmailUploadStatus uploadCtx = {
    .sequence = sequence,
    .characters = NULL,
    .length = 0,
  };

  curlGlobalInit();
  CURL* curl = curl_easy_init();
  if (curl == NULL) {
    curlGlobalCleanup();
    return -NQ_EIO;
  }

  struct curl_slist* recipients = NULL;

  curl_easy_setopt(curl, CURLOPT_URL, params->smtp);
  curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
  curl_easy_setopt(curl, CURLOPT_USERNAME, params->from);
  curl_easy_setopt(curl, CURLOPT_PASSWORD, params->password);

  curl_easy_setopt(curl, CURLOPT_MAIL_FROM, params->from);
  recipients = curl_slist_append(recipients, params->to);
  if (recipients == NULL) {
    curl_easy_cleanup(curl);
    curlGlobalCleanup();
    return -NQ_ENOMEM;
  }

  curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

  curl_easy_setopt(curl, CURLOPT_READFUNCTION, emailPayloadSource);
  curl_easy_setopt(curl, CURLOPT_READDATA, &uploadCtx);
  curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

  int ret = 0;
  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    NQ_LOGE("curl_easy_perform failed: %s (%i)", curl_easy_strerror(res), (int)res);
    ret = -NQ_EIO;
  }

  curl_slist_free_all(recipients);
  curl_easy_cleanup(curl);
  curlGlobalCleanup();
  return ret;
}

#endif
