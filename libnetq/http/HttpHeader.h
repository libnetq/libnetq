/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_HTTP_HTTPHEADER_H
#define _LIBNETQ_HTTP_HTTPHEADER_H

#include <libnetq/string/StringRange.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_HTTP_CRLF "\r\n"

#define NQHTTP_HEADER_ACCEPT                       "Accept"
#define NQHTTP_HEADER_ACCEPT_CH                    "Accept-CH"
#define NQHTTP_HEADER_ACCEPT_ENCODING              "Accept-Encoding"
#define NQHTTP_HEADER_ACCEPT_LANGUAGE              "Accept-Language"
#define NQHTTP_HEADER_AUTHORIZATION                "Authorization"
#define NQHTTP_HEADER_CACHE_CONTROL                "Cache-Control"
#define NQHTTP_HEADER_CONNECTION                   "Connection"
#define NQHTTP_HEADER_CONTENT_ENCODING             "Content-Encoding"
#define NQHTTP_HEADER_CONTENT_DISPOSITION          "Content-Disposition"
#define NQHTTP_HEADER_CONTENT_LENGTH               "Content-Length"
#define NQHTTP_HEADER_CONTENT_MD5                  "Content-MD5"
#define NQHTTP_HEADER_CONTENT_SECURITY_POLICY      "Content-Security-Policy"
#define NQHTTP_HEADER_CONTENT_TRANSFER_ENCODING    "Content-Transfer-Encoding"
#define NQHTTP_HEADER_CONTENT_TYPE                 "Content-Type"
#define NQHTTP_HEADER_COOKIE                       "Cookie"
#define NQHTTP_HEADER_CROSS_ORIGIN_EMBEDDER_POLICY "Cross-Origin-Embedder-Policy"
#define NQHTTP_HEADER_CROSS_ORIGIN_OPENER_POLICY   "Cross-Origin-Opener-Policy"
#define NQHTTP_HEADER_DATE                         "Date"
#define NQHTTP_HEADER_HOST                         "Host"
#define NQHTTP_HEADER_LOCATION                     "Location"
#define NQHTTP_HEADER_ORIGIN                       "Origin"
#define NQHTTP_HEADER_PRAGMA                       "Pragma"
#define NQHTTP_HEADER_SEC_FETCH_DEST               "Sec-Fetch-Dest"
#define NQHTTP_HEADER_SEC_FETCH_MODE               "Sec-Fetch-Mode"
#define NQHTTP_HEADER_SEC_FETCH_SITE               "Sec-Fetch-Site"
#define NQHTTP_HEADER_SEC_FETCH_USER               "Sec-Fetch-User"
#define NQHTTP_HEADER_SEC_PURPOSE                  "Sec-Purpose"
#define NQHTTP_HEADER_SEC_WEBSOCKET_ACCEPT         "Sec-WebSocket-Accept"
#define NQHTTP_HEADER_SEC_WEBSOCKET_EXTENSIONS     "Sec-WebSocket-Extensions"
#define NQHTTP_HEADER_SEC_WEBSOCKET_KEY            "Sec-WebSocket-Key"
#define NQHTTP_HEADER_SEC_WEBSOCKET_PROTOCOL       "Sec-WebSocket-Protocol"
#define NQHTTP_HEADER_SEC_WEBSOCKET_VERSION        "Sec-WebSocket-Version"
#define NQHTTP_HEADER_SERVER                       "Server"
#define NQHTTP_HEADER_UPGRADE                      "Upgrade"
#define NQHTTP_HEADER_UPGRADE_INSECURE_REQUESTS    "Upgrade-Insecure-Requests"
#define NQHTTP_HEADER_USER_AGENT                   "User-Agent"

#define NQHttpHeaderIsNonStandard(s) ((s)[0] == 'X' && (s)[1] == '-')

typedef struct NQHttpRequestLine NQHttpRequestLine;
struct NQHttpRequestLine {
  NQStringRange method;
  NQStringRange url;
  NQStringRange version;
};

NQ_EXPORT bool NQHttpRequestLineParse(const char* data, size_t size, NQHttpRequestLine* result);

typedef struct NQHttpStatusLine NQHttpStatusLine;
struct NQHttpStatusLine {
  NQStringRange version;
  NQStringRange code;
  NQStringRange reason;
};

NQ_EXPORT bool NQHttpStatusLineParse(const char* data, size_t size, NQHttpStatusLine* result);

typedef struct NQHttpHeaderLine NQHttpHeaderLine;
struct NQHttpHeaderLine {
  NQStringRange name;
  NQStringRange value;
};

NQ_EXPORT bool NQHttpHeaderLineParse(const char* data, size_t size, NQHttpHeaderLine* result);

typedef struct NQHttpHeaderValue NQHttpHeaderValue;
struct NQHttpHeaderValue {
  NQStringRange key;
  NQStringRange value;
};

NQ_EXPORT bool NQHttpHeaderValueParse(const char* data, size_t size, NQHttpHeaderValue* result);

typedef struct NQHttpFormData NQHttpFormData;
struct NQHttpFormData {
  NQStringRange name;
  NQStringRange filename;
};

NQ_EXPORT bool NQHttpFormDataParse(const char* data, size_t size, NQHttpFormData* result);

NQ_EXPORT const char* NQHttpGetContentBoundary(const char* contentType);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_HTTP_HTTPHEADER_H */
