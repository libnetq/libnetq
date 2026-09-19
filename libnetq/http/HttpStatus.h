/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_HTTP_HTTPSTATUS_H
#define _LIBNETQ_HTTP_HTTPSTATUS_H

/* 1xx – Informational Responses */
#define NQ_HTTP_CONTINUE                      100
#define NQ_HTTP_SWITCHING_PROTOCOLS           101
#define NQ_HTTP_PROCESSING                    102
#define NQ_HTTP_EARLY_HINTS                   103

/* 2xx – Success */
#define NQ_HTTP_OK                            200
#define NQ_HTTP_CREATED                       201
#define NQ_HTTP_ACCEPTED                      202
#define NQ_HTTP_NO_CONTENT                    204
#define NQ_HTTP_RESET_CONTENT                 205
#define NQ_HTTP_PARTIAL_CONTENT               206

/* 3xx – Redirection */
#define NQ_HTTP_MOVED_PERMANENTLY             301
#define NQ_HTTP_MOVED_TEMPORARILY             302
#define NQ_HTTP_SEE_OTHER                     303
#define NQ_HTTP_NOT_MODIFIED                  304
#define NQ_HTTP_USE_PROXY                     305
#define NQ_HTTP_TEMPORARY_REDIRECT            307

/* 4xx – Client Errors */
#define NQ_HTTP_BAD_REQUEST                   400
#define NQ_HTTP_UNAUTHORIZED                  401
#define NQ_HTTP_NOT_FOUND                     404
#define NQ_HTTP_NOT_ALLOWED                   405
#define NQ_HTTP_PROXY_AUTHENTICATION_REQUIRED 407
#define NQ_HTTP_PRECONDITION_FAILED           412
#define NQ_HTTP_REQUEST_ENTITY_TOO_LARGE      413
#define NQ_HTTP_RANGE_NOT_SATISFIABLE         416
#define NQ_HTTP_EXPECTATION_FAILED            417
#define NQ_HTTP_UPGRADE_REQUIRED              426

/* 5xx – Server Errors */
#define NQ_HTTP_INTERNAL_SERVER_ERROR         500
#define NQ_HTTP_NOT_IMPLEMENTED               501
#define NQ_HTTP_BAD_GATEWAY                   502
#define NQ_HTTP_SERVICE_UNAVAILABLE           503

static inline bool NQIsHttpStatusInformational(int code)
{
  return 100 <= code && code < 200;
}

static inline bool NQIsHttpStatusSuccess(int code)
{
  return 200 <= code && code < 300;
}

static inline bool NQIsHttpStatusRedirection(int code)
{
  return 300 <= code && code < 400;
}

static inline bool NQIsHttpStatusClientError(int code)
{
  return 400 <= code && code < 500;
}

static inline bool NQIsHttpStatusServerError(int code)
{
  return 500 <= code && code < 600;
}

#endif /* _LIBNETQ_HTTP_HTTPSTATUS_H */
