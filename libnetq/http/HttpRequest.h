/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_HTTP_HTTPREQUEST_H
#define _LIBNETQ_HTTP_HTTPREQUEST_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQHttpRequest NQHttpRequest;
typedef struct NQHttpRequestHeaderIter NQHttpRequestHeaderIter;

/* Receives the response body as it arrives. Returns 0 to go on; anything
 * else stops the transfer, which then fails with
 * NQ_HTTP_REQUEST_ERROR_ABORTED. */
typedef int (*NQHttpRequestWriteCallback) (void* userdata, const void* data, size_t size);

/* What made the last NQHttpRequest_performSync fail */
typedef enum NQHttpRequestError {
  NQ_HTTP_REQUEST_ERROR_NONE = 0,
  NQ_HTTP_REQUEST_ERROR_OTHER,
  NQ_HTTP_REQUEST_ERROR_URL,      /* malformed URL or unsupported scheme */
  NQ_HTTP_REQUEST_ERROR_RESOLVE,  /* host or proxy name not resolved */
  NQ_HTTP_REQUEST_ERROR_CONNECT,  /* connection refused or unreachable */
  NQ_HTTP_REQUEST_ERROR_TIMEOUT,
  NQ_HTTP_REQUEST_ERROR_NETWORK,  /* sending or receiving failed, transfer cut short */
  NQ_HTTP_REQUEST_ERROR_SSL,      /* TLS handshake */
  NQ_HTTP_REQUEST_ERROR_CERTIFICATE, /* the server's certificate is not trusted */
  NQ_HTTP_REQUEST_ERROR_PROTOCOL, /* malformed or unexpected server reply */
  NQ_HTTP_REQUEST_ERROR_ABORTED,  /* the write callback stopped the transfer */
} NQHttpRequestError;

/* Receives each response header line as it arrives, line end included:
 * status lines ("HTTP/1.1 200 OK\r\n") and the empty line that ends the
 * headers too, of every response (interim ones and redirects followed).
 * Returns 0 to go on; anything else stops the transfer as the write
 * callback does. */
typedef int (*NQHttpRequestHeaderCallback) (void* userdata, const char* line, size_t size);

/* What a debug callback is given */
typedef enum NQHttpRequestDebugType {
  NQ_HTTP_REQUEST_DEBUG_TEXT = 0, /* what the library is doing, a line of text */
  NQ_HTTP_REQUEST_DEBUG_HEADER_IN,
  NQ_HTTP_REQUEST_DEBUG_HEADER_OUT,
  NQ_HTTP_REQUEST_DEBUG_DATA_IN,
  NQ_HTTP_REQUEST_DEBUG_DATA_OUT,
} NQHttpRequestDebugType;

typedef void (*NQHttpRequestDebugCallback) (void* userdata, NQHttpRequestDebugType type, const char* data, size_t size);

/* Ways to send credentials, for NQHttpRequest_setAuthMethods */
enum {
  NQ_HTTP_AUTH_BASIC = 1 << 0,
  NQ_HTTP_AUTH_DIGEST = 1 << 1,
  NQ_HTTP_AUTH_NTLM = 1 << 2,
  NQ_HTTP_AUTH_NEGOTIATE = 1 << 3,
  NQ_HTTP_AUTH_ANY = 0xff,
};

/* Time from the start of a transfer to each of its stages, in
 * microseconds, as libcurl's CURLINFO_*_TIME_T report them */
typedef struct NQHttpRequestTimings {
  int64_t nameLookup;
  int64_t connect;
  int64_t appConnect;    /* TLS handshake done */
  int64_t preTransfer;
  int64_t startTransfer; /* first response byte */
  int64_t redirect;      /* spent on the redirects followed */
  int64_t total;
} NQHttpRequestTimings;

/* The HTTP backend and its libraries, as in "libcurl/8.17.0 OpenSSL/3.4.0
 * zlib/1.3.1" */
NQ_EXPORT const char* NQHttpRequestBackendVersion(void);

NQ_EXPORT NQHttpRequest* NQHttpRequest_create(NQHttpRequestWriteCallback callback, void* userdata);
NQ_EXPORT void NQHttpRequest_release(NQHttpRequest*);

NQ_EXPORT int NQHttpRequest_performSync(NQHttpRequest*);
NQ_EXPORT const char* NQHttpRequest_lastErrorMessage(NQHttpRequest*);
NQ_EXPORT NQHttpRequestError NQHttpRequest_lastError(NQHttpRequest*);
/* The backend's own code for the last result: a CURLcode, 0 for success */
NQ_EXPORT int NQHttpRequest_lastErrorCode(NQHttpRequest*);

NQ_EXPORT bool NQHttpRequest_setHeaderCallback(NQHttpRequest*, NQHttpRequestHeaderCallback callback, void* userdata);
/* Turns on tracing what is sent and received; NULL turns it off */
NQ_EXPORT bool NQHttpRequest_setDebugCallback(NQHttpRequest*, NQHttpRequestDebugCallback callback, void* userdata);

NQ_EXPORT bool NQHttpRequest_setUrl(NQHttpRequest*, const char* url);
NQ_EXPORT bool NQHttpRequest_setMethod(NQHttpRequest*, const char* method);
/* Sends `method` as the request's verb while everything else (the body
 * set, what is expected back) stays as the other settings make it, as
 * curl's -X does; NULL goes back to the verb those settings imply */
NQ_EXPORT bool NQHttpRequest_setCustomMethod(NQHttpRequest*, const char* method);
NQ_EXPORT bool NQHttpRequest_setPostData(NQHttpRequest*, const void* data, size_t size);
NQ_EXPORT bool NQHttpRequest_setFollowLocation(NQHttpRequest*, bool value);
/* How many redirects NQHttpRequest_setFollowLocation follows; -1 for no
 * limit */
NQ_EXPORT bool NQHttpRequest_setMaxRedirects(NQHttpRequest*, long count);
/* Sends `size` bytes as the body of an upload (PUT unless another method
 * is set), without the form encoding NQHttpRequest_setPostData implies.
 * The data must stay valid until the transfer is done. */
NQ_EXPORT bool NQHttpRequest_setUploadData(NQHttpRequest*, const void* data, size_t size);
/* Adds a part to a multipart/form-data body ("name=value"): `data` (copied),
 * sent as a file upload when `filename` is set; `contentType` may be NULL */
NQ_EXPORT bool NQHttpRequest_addFormPart(NQHttpRequest*, const char* name, const void* data, size_t size,
                                         const char* filename, const char* contentType);
/* Adds the file at `path` as a part of a multipart/form-data body, under
 * its own name or `filename` */
NQ_EXPORT bool NQHttpRequest_addFormFile(NQHttpRequest*, const char* name, const char* path,
                                         const char* filename, const char* contentType);
/* Asks for a part of the resource: "0-99", "500-", "-200" */
NQ_EXPORT bool NQHttpRequest_setRange(NQHttpRequest*, const char* range);
/* Goes on from `offset` bytes into the resource; fails with the
 * backend's "cannot resume" code when the server sends all of it */
NQ_EXPORT bool NQHttpRequest_setResumeFrom(NQHttpRequest*, int64_t offset);
/* Asks for a compressed body ("" for every encoding the backend knows)
 * and decodes it; NULL asks for none */
NQ_EXPORT bool NQHttpRequest_setAcceptEncoding(NQHttpRequest*, const char* encodings);
/* "[scheme://]host[:port]"; NULL for the environment's (http_proxy & co.) */
NQ_EXPORT bool NQHttpRequest_setProxy(NQHttpRequest*, const char* proxy);
/* Cookies: sent as they are ("a=1; b=2"), read from a cookie file ("" to
 * just keep the ones received), and written to a jar when released */
NQ_EXPORT bool NQHttpRequest_setCookie(NQHttpRequest*, const char* cookies);
NQ_EXPORT bool NQHttpRequest_setCookieFile(NQHttpRequest*, const char* path);
NQ_EXPORT bool NQHttpRequest_setCookieJar(NQHttpRequest*, const char* path);
NQ_EXPORT bool NQHttpRequest_setVerifyingPeerSsl(NQHttpRequest* thiz, bool value);
NQ_EXPORT bool NQHttpRequest_setVerifyingHostSsl(NQHttpRequest* thiz, bool value);

static inline bool NQHttpRequest_setVerifyingSsl(NQHttpRequest* thiz, bool value)
{
  bool peerOk = NQHttpRequest_setVerifyingPeerSsl(thiz, value);
  bool hostOk = NQHttpRequest_setVerifyingHostSsl(thiz, value);
  return peerOk && hostOk;
}

/* The whole transfer; 0 means no limit */
NQ_EXPORT bool NQHttpRequest_setTimeoutMs(NQHttpRequest* thiz, int64_t timeoutMs);
/* User name and password, sent the way the server asks for them (Basic,
 * Digest, ...) after it answers 401; NULL user name sends none */
NQ_EXPORT bool NQHttpRequest_setCredentials(NQHttpRequest* thiz, const char* username, const char* password);
/* Restricts the ways credentials are sent (NQ_HTTP_AUTH_*); Basic alone
 * sends them with the first request. Call after setCredentials. */
NQ_EXPORT bool NQHttpRequest_setAuthMethods(NQHttpRequest* thiz, unsigned methods);
/* Name resolution and connecting; 0 means the default */
NQ_EXPORT bool NQHttpRequest_setConnectTimeoutMs(NQHttpRequest* thiz, int64_t timeoutMs);
/* Gives up when no data arrives for that long (rounded up to seconds);
 * 0 means no limit */
NQ_EXPORT bool NQHttpRequest_setIdleTimeoutMs(NQHttpRequest* thiz, int64_t timeoutMs);
/* Request headers are sent in the order added; one added with an empty
 * value removes a header the library would send ("Accept:") */
NQ_EXPORT bool NQHttpRequest_addHeader(NQHttpRequest*, const char* name, const char* value);
/* Adds a header line as it is: "Name: value", "Name:" to remove a header
 * the library would send, "Name;" to send one with an empty value */
NQ_EXPORT bool NQHttpRequest_addHeaderLine(NQHttpRequest*, const char* line);

/* The status of the final response: after interim ones (100 Continue)
 * and, with NQHttpRequest_setFollowLocation, after the redirects. The
 * response headers are that response's too. */
NQ_EXPORT int NQHttpRequest_responseStatusCode(NQHttpRequest*);
NQ_EXPORT const char* NQHttpRequest_responseReasonText(NQHttpRequest*);
/* The protocol of the status line, as in "HTTP/1.1" or "HTTP/2" */
NQ_EXPORT const char* NQHttpRequest_responseVersion(NQHttpRequest*);
NQ_EXPORT bool NQHttpRequest_responseTimeMs(NQHttpRequest*, int64_t* result);
/* The absolute URL a redirect response points to (its Location resolved
 * against the request URL), or NULL when it is not a redirect. Valid
 * until the request is released or performed again. */
NQ_EXPORT const char* NQHttpRequest_redirectUrl(NQHttpRequest*);
/* The numeric address and port of the server (or proxy) last connected
 * to, or NULL and -1 when there was none */
NQ_EXPORT const char* NQHttpRequest_remoteAddress(NQHttpRequest*);
NQ_EXPORT int NQHttpRequest_remotePort(NQHttpRequest*);
/* The URL last fetched, after the redirects followed */
NQ_EXPORT const char* NQHttpRequest_effectiveUrl(NQHttpRequest*);
NQ_EXPORT long NQHttpRequest_redirectCount(NQHttpRequest*);
NQ_EXPORT bool NQHttpRequest_timings(NQHttpRequest*, NQHttpRequestTimings* result);
NQ_EXPORT size_t NQHttpRequest_responseHeaderCount(NQHttpRequest*);
NQ_EXPORT const char* NQHttpRequest_responseHeaderNameAt(NQHttpRequest*, size_t index);
NQ_EXPORT const char* NQHttpRequest_responseHeaderValueAt(NQHttpRequest*, size_t index);

NQ_EXPORT NQHttpRequestHeaderIter* NQHttpRequest_responseHeaderFirst(NQHttpRequest*);
NQ_EXPORT NQHttpRequestHeaderIter* NQHttpRequest_responseHeaderNext(NQHttpRequest*, NQHttpRequestHeaderIter* iter);
NQ_EXPORT const char* NQHttpRequestHeaderIter_name(NQHttpRequestHeaderIter* iter);
NQ_EXPORT const char* NQHttpRequestHeaderIter_value(NQHttpRequestHeaderIter* iter);

struct NQSendEmailParams {
  const char* smtp;
  const char* username;
  const char* password;
  const char* from;
  const char* to;
  const char* subject;
  const char* content;
};

NQ_EXPORT int NQSendEmailSync(struct NQSendEmailParams* params);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_HTTP_HTTPREQUEST_H */
