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

#ifdef HTTPREQUEST_BACKEND_CURL

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
  NQHttpRequestHeaderCallback headerCallback;
  void* headerUserdata;
  NQHttpRequestDebugCallback debugCallback;
  void* debugUserdata;
  const char* upload;
  size_t uploadSize;
  size_t uploadPos;
  curl_mime* mime;
  char errorBuffer[CURL_ERROR_SIZE];
  CURLcode lastCode;
  bool aborted;
  int state;
  int statucCode;
  NQStringData statusVersion;
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

static void clearResponse(NQHttpRequest* thiz)
{
  while (!NQListHead_isEmpty(&thiz->responseHeaders)) {
    NQHttpRequestHeaderIter* iter = NQ_CONTAINER_OF(thiz->responseHeaders.prev, struct NQHttpRequestHeaderIter, list);
    NQListHead_remove(&iter->list);
    NQFree(iter);
  }
  thiz->statucCode = -1;
  NQStringData_set2(&thiz->statusVersion, "", 0);
  NQStringData_set2(&thiz->statusReason, "", 0);
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
  if (!NQStringData_set2(&thiz->statusVersion, statusLine.version.characters, statusLine.version.length))
    return false;
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
  NQMemcpy(hdrIter->name, headerLine.name.characters, headerLine.name.length);
  hdrIter->name[headerLine.name.length] = '\0';
  NQMemcpy(hdrIter->value, headerLine.value.characters, headerLine.value.length);
  hdrIter->value[headerLine.value.length] = '\0';
  NQ_ASSERT(hdrIter->name + headerLine.name.length + 1 == (char*)hdrIter + sizeInBytes);
  NQListHead_addBack(&thiz->responseHeaders, &hdrIter->list);

  return true;
}

static size_t headerCallback(char* str, size_t size, size_t n, void* userdata)
{
  NQHttpRequest* thiz = (NQHttpRequest*)userdata;
  size_t len = size * n;

  if (thiz->headerCallback && thiz->headerCallback(thiz->headerUserdata, str, len) != 0) {
    thiz->aborted = true;
    return 0;
  }

  size_t sz = len;
  if (sz >= NQ_CSTR_LENGTH(NQ_HTTP_CRLF) && NQMemcmp(str + sz - NQ_CSTR_LENGTH(NQ_HTTP_CRLF), NQ_HTTP_CRLF, NQ_CSTR_LENGTH(NQ_HTTP_CRLF)) == 0)
    sz -= NQ_CSTR_LENGTH(NQ_HTTP_CRLF);

  /* Another response follows an interim one (100 Continue) or a redirect
   * that is followed: only the last one's status and headers are kept */
  if ((thiz->state == DoneState || thiz->state == ErrorState)
      && sz >= NQ_CSTR_LENGTH("HTTP/") && NQMemcmp(str, "HTTP/", NQ_CSTR_LENGTH("HTTP/")) == 0) {
    clearResponse(thiz);
    thiz->state = StatusState;
  }

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
  if (thiz->callback(thiz->userdata, str, len) != 0) {
    thiz->aborted = true;
    return 0; /* anything but len fails the transfer with CURLE_WRITE_ERROR */
  }
  return len;
}

static size_t uploadCallback(char* buffer, size_t size, size_t n, void* userdata)
{
  NQHttpRequest* thiz = (NQHttpRequest*)userdata;
  size_t len = NQGetMin(size * n, thiz->uploadSize - thiz->uploadPos);
  NQMemcpy(buffer, thiz->upload + thiz->uploadPos, len);
  thiz->uploadPos += len;
  return len;
}

static int debugCallback(CURL* curl, curl_infotype type, char* data, size_t size, void* userdata)
{
  NQHttpRequest* thiz = (NQHttpRequest*)userdata;
  NQ_UNUSED_PARAM(curl);
  switch (type) {
  case CURLINFO_TEXT:
    thiz->debugCallback(thiz->debugUserdata, NQ_HTTP_REQUEST_DEBUG_TEXT, data, size);
    break;
  case CURLINFO_HEADER_IN:
    thiz->debugCallback(thiz->debugUserdata, NQ_HTTP_REQUEST_DEBUG_HEADER_IN, data, size);
    break;
  case CURLINFO_HEADER_OUT:
    thiz->debugCallback(thiz->debugUserdata, NQ_HTTP_REQUEST_DEBUG_HEADER_OUT, data, size);
    break;
  case CURLINFO_DATA_IN:
    thiz->debugCallback(thiz->debugUserdata, NQ_HTTP_REQUEST_DEBUG_DATA_IN, data, size);
    break;
  case CURLINFO_DATA_OUT:
    thiz->debugCallback(thiz->debugUserdata, NQ_HTTP_REQUEST_DEBUG_DATA_OUT, data, size);
    break;
  default: /* TLS records */
    break;
  }
  return 0;
}

static int toErrorCode(CURLcode res)
{
  if (res != CURLE_OK)
    return -NQ_EIO;
  return 0;
}

static NQHttpRequestError toRequestError(CURLcode res, bool aborted)
{
  switch (res) {
  case CURLE_OK:
    return NQ_HTTP_REQUEST_ERROR_NONE;
  case CURLE_UNSUPPORTED_PROTOCOL:
  case CURLE_URL_MALFORMAT:
    return NQ_HTTP_REQUEST_ERROR_URL;
  case CURLE_COULDNT_RESOLVE_PROXY:
  case CURLE_COULDNT_RESOLVE_HOST:
    return NQ_HTTP_REQUEST_ERROR_RESOLVE;
  case CURLE_COULDNT_CONNECT:
    return NQ_HTTP_REQUEST_ERROR_CONNECT;
  case CURLE_OPERATION_TIMEDOUT:
    return NQ_HTTP_REQUEST_ERROR_TIMEOUT;
  case CURLE_PARTIAL_FILE:
  case CURLE_SEND_ERROR:
  case CURLE_RECV_ERROR:
  case CURLE_GOT_NOTHING:
    return NQ_HTTP_REQUEST_ERROR_NETWORK;
  case CURLE_SSL_CONNECT_ERROR:
  case CURLE_SSL_CERTPROBLEM:
  case CURLE_SSL_CIPHER:
  case CURLE_SSL_CACERT_BADFILE:
    return NQ_HTTP_REQUEST_ERROR_SSL;
  case CURLE_PEER_FAILED_VERIFICATION:
  case CURLE_SSL_ISSUER_ERROR:
  case CURLE_SSL_PINNEDPUBKEYNOTMATCH:
  case CURLE_SSL_INVALIDCERTSTATUS:
    return NQ_HTTP_REQUEST_ERROR_CERTIFICATE;
  case CURLE_WEIRD_SERVER_REPLY:
  case CURLE_HTTP2:
  case CURLE_HTTP2_STREAM:
  case CURLE_TOO_MANY_REDIRECTS:
  case CURLE_BAD_CONTENT_ENCODING:
    return NQ_HTTP_REQUEST_ERROR_PROTOCOL;
  case CURLE_WRITE_ERROR:
    return aborted ? NQ_HTTP_REQUEST_ERROR_ABORTED : NQ_HTTP_REQUEST_ERROR_OTHER;
  default:
    return NQ_HTTP_REQUEST_ERROR_OTHER;
  }
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
  thiz->headerCallback = NULL;
  thiz->headerUserdata = NULL;
  thiz->debugCallback = NULL;
  thiz->debugUserdata = NULL;
  thiz->upload = NULL;
  thiz->uploadSize = 0;
  thiz->uploadPos = 0;
  thiz->mime = NULL;

  thiz->errorBuffer[0] = '\0';
  thiz->lastCode = CURLE_OK;
  thiz->aborted = false;

  curl_easy_setopt(thiz->curl, CURLOPT_WRITEFUNCTION, writeCallback);
  curl_easy_setopt(thiz->curl, CURLOPT_WRITEDATA, thiz);

  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, thiz);

  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, thiz->errorBuffer);

  thiz->state = InitState;
  thiz->statucCode = -1;
  NQStringData_init(&thiz->statusVersion);
  NQStringData_init(&thiz->statusReason);
  NQListHead_init(&thiz->responseHeaders);

  return thiz;
}

void NQHttpRequest_release(NQHttpRequest* thiz)
{
  clearResponse(thiz);
  NQStringData_finalize(&thiz->statusVersion);
  NQStringData_finalize(&thiz->statusReason);
  NQCURLHeaders_finalize(&thiz->headers);
  curl_easy_cleanup(thiz->curl);
  curl_mime_free(thiz->mime);
  NQFree(thiz);
  curlGlobalCleanup();
}

int NQHttpRequest_performSync(NQHttpRequest* thiz)
{
  if (thiz->state == InitState)
    thiz->state = StatusState;
  thiz->errorBuffer[0] = '\0';
  thiz->aborted = false;
  thiz->uploadPos = 0;
  curl_easy_setopt(thiz->curl, CURLOPT_HTTPHEADER, thiz->headers.impl);
  if (thiz->mime)
    curl_easy_setopt(thiz->curl, CURLOPT_MIMEPOST, thiz->mime);
  thiz->lastCode = curl_easy_perform(thiz->curl);
  return toErrorCode(thiz->lastCode);
}

const char* NQHttpRequest_lastErrorMessage(NQHttpRequest* thiz)
{
  /* not every failure fills in the error buffer */
  if (thiz->errorBuffer[0] == '\0' && thiz->lastCode != CURLE_OK)
    return curl_easy_strerror(thiz->lastCode);
  return thiz->errorBuffer;
}

NQHttpRequestError NQHttpRequest_lastError(NQHttpRequest* thiz)
{
  return toRequestError(thiz->lastCode, thiz->aborted);
}

int NQHttpRequest_lastErrorCode(NQHttpRequest* thiz)
{
  return (int)thiz->lastCode;
}

const char* NQHttpRequestBackendVersion(void)
{
  return curl_version();
}

bool NQHttpRequest_setHeaderCallback(NQHttpRequest* thiz, NQHttpRequestHeaderCallback callback, void* userdata)
{
  thiz->headerCallback = callback;
  thiz->headerUserdata = userdata;
  return true;
}

bool NQHttpRequest_setDebugCallback(NQHttpRequest* thiz, NQHttpRequestDebugCallback callback, void* userdata)
{
  thiz->debugCallback = callback;
  thiz->debugUserdata = userdata;
  if (callback == NULL) {
    curl_easy_setopt(thiz->curl, CURLOPT_DEBUGFUNCTION, NULL);
    return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_VERBOSE, 0L));
  }
  if (!toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_DEBUGFUNCTION, debugCallback)))
    return false;
  if (!toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_DEBUGDATA, thiz)))
    return false;
  return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_VERBOSE, 1L));
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

bool NQHttpRequest_setCustomMethod(NQHttpRequest* thiz, const char* method)
{
  return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_CUSTOMREQUEST, method));
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

bool NQHttpRequest_setMaxRedirects(NQHttpRequest* thiz, long count)
{
  return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_MAXREDIRS, count));
}

bool NQHttpRequest_setUploadData(NQHttpRequest* thiz, const void* data, size_t size)
{
  thiz->upload = (const char*)data;
  thiz->uploadSize = size;
  thiz->uploadPos = 0;
  if (!toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_READFUNCTION, uploadCallback)))
    return false;
  if (!toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_READDATA, thiz)))
    return false;
  if (!toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)size)))
    return false;
  return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_UPLOAD, 1L));
}

static curl_mimepart* addFormPart(NQHttpRequest* thiz, const char* name)
{
  if (thiz->mime == NULL) {
    thiz->mime = curl_mime_init(thiz->curl);
    if (thiz->mime == NULL)
      return NULL;
  }
  curl_mimepart* part = curl_mime_addpart(thiz->mime);
  if (part == NULL || curl_mime_name(part, name) != CURLE_OK)
    return NULL;
  return part;
}

bool NQHttpRequest_addFormPart(NQHttpRequest* thiz, const char* name, const void* data, size_t size,
                               const char* filename, const char* contentType)
{
  curl_mimepart* part = addFormPart(thiz, name);
  if (part == NULL || curl_mime_data(part, (const char*)data, size) != CURLE_OK)
    return false;
  if (filename && curl_mime_filename(part, filename) != CURLE_OK)
    return false;
  if (contentType && curl_mime_type(part, contentType) != CURLE_OK)
    return false;
  return true;
}

bool NQHttpRequest_addFormFile(NQHttpRequest* thiz, const char* name, const char* path,
                               const char* filename, const char* contentType)
{
  curl_mimepart* part = addFormPart(thiz, name);
  if (part == NULL || curl_mime_filedata(part, path) != CURLE_OK)
    return false;
  if (filename && curl_mime_filename(part, filename) != CURLE_OK)
    return false;
  if (contentType && curl_mime_type(part, contentType) != CURLE_OK)
    return false;
  return true;
}

bool NQHttpRequest_setRange(NQHttpRequest* thiz, const char* range)
{
  return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_RANGE, range));
}

bool NQHttpRequest_setResumeFrom(NQHttpRequest* thiz, int64_t offset)
{
  return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)offset));
}

bool NQHttpRequest_setAcceptEncoding(NQHttpRequest* thiz, const char* encodings)
{
  return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_ACCEPT_ENCODING, encodings));
}

bool NQHttpRequest_setProxy(NQHttpRequest* thiz, const char* proxy)
{
  return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_PROXY, proxy));
}

bool NQHttpRequest_setCookie(NQHttpRequest* thiz, const char* cookies)
{
  return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_COOKIE, cookies));
}

bool NQHttpRequest_setCookieFile(NQHttpRequest* thiz, const char* path)
{
  return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_COOKIEFILE, path));
}

bool NQHttpRequest_setCookieJar(NQHttpRequest* thiz, const char* path)
{
  return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_COOKIEJAR, path));
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

bool NQHttpRequest_setCredentials(NQHttpRequest* thiz, const char* username, const char* password)
{
  if (!toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_USERNAME, username)))
    return false;
  if (!toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_PASSWORD, username ? password : NULL)))
    return false;
  return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_HTTPAUTH, username ? (long)CURLAUTH_ANY : (long)CURLAUTH_BASIC));
}

bool NQHttpRequest_setAuthMethods(NQHttpRequest* thiz, unsigned methods)
{
  unsigned long mask = 0;
  if (methods == NQ_HTTP_AUTH_ANY)
    mask = CURLAUTH_ANY;
  else {
    if (methods & NQ_HTTP_AUTH_BASIC)
      mask |= CURLAUTH_BASIC;
    if (methods & NQ_HTTP_AUTH_DIGEST)
      mask |= CURLAUTH_DIGEST;
    if (methods & NQ_HTTP_AUTH_NTLM)
      mask |= CURLAUTH_NTLM;
    if (methods & NQ_HTTP_AUTH_NEGOTIATE)
      mask |= CURLAUTH_NEGOTIATE;
  }
  return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_HTTPAUTH, mask));
}

bool NQHttpRequest_setConnectTimeoutMs(NQHttpRequest* thiz, int64_t timeoutMs)
{
  timeoutMs = NQGetClamp(timeoutMs, 0, NQ_LONG_MAX);
  return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_CONNECTTIMEOUT_MS, (long)timeoutMs));
}

bool NQHttpRequest_setIdleTimeoutMs(NQHttpRequest* thiz, int64_t timeoutMs)
{
  /* "less than a byte a second for that many seconds" */
  timeoutMs = NQGetClamp(timeoutMs, 0, NQ_LONG_MAX - 999);
  long seconds = (long)((timeoutMs + 999) / 1000);
  if (!toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_LOW_SPEED_LIMIT, seconds ? 1L : 0L)))
    return false;
  return toBoolean(curl_easy_setopt(thiz->curl, CURLOPT_LOW_SPEED_TIME, seconds));
}

bool NQHttpRequest_addHeader(NQHttpRequest* thiz, const char* name, const char* value)
{
  return NQCURLHeaders_add(&thiz->headers, name, value);
}

bool NQHttpRequest_addHeaderLine(NQHttpRequest* thiz, const char* line)
{
  return NQCURLHeaders_addLine(&thiz->headers, line);
}

int NQHttpRequest_responseStatusCode(NQHttpRequest* thiz)
{
  return thiz->statucCode;
}

const char* NQHttpRequest_responseReasonText(NQHttpRequest* thiz)
{
  return NQStringData_characters(&thiz->statusReason);
}

const char* NQHttpRequest_responseVersion(NQHttpRequest* thiz)
{
  return NQStringData_characters(&thiz->statusVersion);
}

bool NQHttpRequest_responseTimeMs(NQHttpRequest* thiz, int64_t* result)
{
  curl_off_t totalTimeUs = 0;
  if (!toBoolean(curl_easy_getinfo(thiz->curl, CURLINFO_TOTAL_TIME_T, &totalTimeUs)))
      return false;
  *result = (int64_t)(totalTimeUs / 1000);
  return true;
}

const char* NQHttpRequest_redirectUrl(NQHttpRequest* thiz)
{
  char* url = NULL;
  if (!toBoolean(curl_easy_getinfo(thiz->curl, CURLINFO_REDIRECT_URL, &url)))
    return NULL;
  return url;
}

const char* NQHttpRequest_remoteAddress(NQHttpRequest* thiz)
{
  char* ip = NULL;
  if (!toBoolean(curl_easy_getinfo(thiz->curl, CURLINFO_PRIMARY_IP, &ip)) || ip == NULL || ip[0] == '\0')
    return NULL;
  return ip;
}

int NQHttpRequest_remotePort(NQHttpRequest* thiz)
{
  long port = 0;
  if (!toBoolean(curl_easy_getinfo(thiz->curl, CURLINFO_PRIMARY_PORT, &port)) || port <= 0)
    return -1;
  return (int)port;
}

const char* NQHttpRequest_effectiveUrl(NQHttpRequest* thiz)
{
  char* url = NULL;
  if (!toBoolean(curl_easy_getinfo(thiz->curl, CURLINFO_EFFECTIVE_URL, &url)))
    return NULL;
  return url;
}

long NQHttpRequest_redirectCount(NQHttpRequest* thiz)
{
  long count = 0;
  if (!toBoolean(curl_easy_getinfo(thiz->curl, CURLINFO_REDIRECT_COUNT, &count)))
    return 0;
  return count;
}

static int64_t timeInfo(CURL* curl, CURLINFO info)
{
  curl_off_t us = 0;
  if (curl_easy_getinfo(curl, info, &us) != CURLE_OK)
    return 0;
  return (int64_t)us;
}

bool NQHttpRequest_timings(NQHttpRequest* thiz, NQHttpRequestTimings* result)
{
  result->nameLookup = timeInfo(thiz->curl, CURLINFO_NAMELOOKUP_TIME_T);
  result->connect = timeInfo(thiz->curl, CURLINFO_CONNECT_TIME_T);
  result->appConnect = timeInfo(thiz->curl, CURLINFO_APPCONNECT_TIME_T);
  result->preTransfer = timeInfo(thiz->curl, CURLINFO_PRETRANSFER_TIME_T);
  result->startTransfer = timeInfo(thiz->curl, CURLINFO_STARTTRANSFER_TIME_T);
  result->redirect = timeInfo(thiz->curl, CURLINFO_REDIRECT_TIME_T);
  result->total = timeInfo(thiz->curl, CURLINFO_TOTAL_TIME_T);
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
  NQMemcpy(ptr, uploadCtx->characters, n);
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
  curl_easy_setopt(curl, CURLOPT_USERNAME, params->username);
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
