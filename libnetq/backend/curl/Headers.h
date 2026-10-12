/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_BACKEND_CURL_HEADERS_H
#define _LIBNETQ_BACKEND_CURL_HEADERS_H

#include <libnetq/backend/curl/Adapter.h>

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
NQ_EXPORT bool NQCURLHeaders_addLine(NQCURLHeaders*, const char* line);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_BACKEND_CURL_HEADERS_H */
