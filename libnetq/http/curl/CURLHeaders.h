/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_HTTP_CURL_CURLHEADERS_H
#define _LIBNETQ_HTTP_CURL_CURLHEADERS_H

#include <libnetq/Basic.h>
#include <libnetq/http/curl/CURLAdapter.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQCURLHeaders NQCURLHeaders;
struct NQCURLHeaders {
  struct curl_slist* impl;
};

static inline void NQCURLHeaders_init(NQCURLHeaders* thiz)
{
  thiz->impl = NULL;
}

NQ_EXPORT void NQCURLHeaders_finalize(NQCURLHeaders*);
NQ_EXPORT bool NQCURLHeaders_add(NQCURLHeaders*, const char* name, const char* value);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_HTTP_CURL_CURLHEADERS_H */
