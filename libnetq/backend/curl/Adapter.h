/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_BACKEND_CURL_ADAPTER_H
#define _LIBNETQ_BACKEND_CURL_ADAPTER_H

#include <libnetq/Basic.h>

#include <curl/curl.h>

#if LIBCURL_VERSION_NUM < 0x073100
/* CURLE_HTTP2_STREAM was added in curl 7.49.0; older versions never report it */
#define CURLE_HTTP2_STREAM ((CURLcode)92)
#endif

#if LIBCURL_VERSION_NUM < 0x073300
/* CURLE_FTP_WEIRD_SERVER_REPLY was renamed to CURLE_WEIRD_SERVER_REPLY in curl 7.51.0 */
#define CURLE_WEIRD_SERVER_REPLY CURLE_FTP_WEIRD_SERVER_REPLY
#endif

#if LIBCURL_VERSION_NUM < 0x073800
/* MIME API was added in curl 7.56.0; multipart forms are unsupported on older versions */
typedef struct curl_mime_s curl_mime;
typedef struct curl_mimepart_s curl_mimepart;
#define CURLOPT_MIMEPOST ((CURLoption)(CURLOPTTYPE_OBJECTPOINT + 269))
static inline curl_mime* curl_mime_init(CURL* easy)
{
  NQ_UNUSED_PARAM(easy);
  return NULL;
}
static inline void curl_mime_free(curl_mime* mime)
{
  NQ_UNUSED_PARAM(mime);
}
static inline curl_mimepart* curl_mime_addpart(curl_mime* mime)
{
  NQ_UNUSED_PARAM(mime);
  return NULL;
}
static inline CURLcode curl_mime_name(curl_mimepart* part, const char* name)
{
  NQ_UNUSED_PARAM(part);
  NQ_UNUSED_PARAM(name);
  return CURLE_NOT_BUILT_IN;
}
static inline CURLcode curl_mime_filename(curl_mimepart* part, const char* filename)
{
  NQ_UNUSED_PARAM(part);
  NQ_UNUSED_PARAM(filename);
  return CURLE_NOT_BUILT_IN;
}
static inline CURLcode curl_mime_type(curl_mimepart* part, const char* mimetype)
{
  NQ_UNUSED_PARAM(part);
  NQ_UNUSED_PARAM(mimetype);
  return CURLE_NOT_BUILT_IN;
}
static inline CURLcode curl_mime_data(curl_mimepart* part, const char* data, size_t datasize)
{
  NQ_UNUSED_PARAM(part);
  NQ_UNUSED_PARAM(data);
  NQ_UNUSED_PARAM(datasize);
  return CURLE_NOT_BUILT_IN;
}
static inline CURLcode curl_mime_filedata(curl_mimepart* part, const char* filename)
{
  NQ_UNUSED_PARAM(part);
  NQ_UNUSED_PARAM(filename);
  return CURLE_NOT_BUILT_IN;
}
#endif

#if LIBCURL_VERSION_NUM < 0x073d00
/* CURLINFO_*_TIME_T were added in curl 7.61.0; older versions report seconds as double */
#define CURLINFO_NAMELOOKUP_TIME_T    CURLINFO_NAMELOOKUP_TIME
#define CURLINFO_CONNECT_TIME_T       CURLINFO_CONNECT_TIME
#define CURLINFO_APPCONNECT_TIME_T    CURLINFO_APPCONNECT_TIME
#define CURLINFO_PRETRANSFER_TIME_T   CURLINFO_PRETRANSFER_TIME
#define CURLINFO_STARTTRANSFER_TIME_T CURLINFO_STARTTRANSFER_TIME
#define CURLINFO_REDIRECT_TIME_T      CURLINFO_REDIRECT_TIME
#define CURLINFO_TOTAL_TIME_T         CURLINFO_TOTAL_TIME
typedef double NQCURLTime;
#define NQCURLTimeToUs(t) (int64_t)((t) * 1000000.0)
#define NQCURLTimeToMs(t) (int64_t)((t) * 1000.0)
#else
typedef curl_off_t NQCURLTime;
#define NQCURLTimeToUs(t) (int64_t)(t)
#define NQCURLTimeToMs(t) (int64_t)((t) / 1000)
#endif

#endif /* _LIBNETQ_BACKEND_CURL_ADAPTER_H */
