/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_HTTP_HTTPSTATISTICS_H
#define _LIBNETQ_HTTP_HTTPSTATISTICS_H

#include <libnetq/json/JSONWriter.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQHttpStatistics NQHttpStatistics;

NQ_EXPORT NQHttpStatistics* NQHttpStatistics_create(void);
NQ_EXPORT void NQHttpStatistics_destroy(NQHttpStatistics*);

NQ_EXPORT bool NQHttpStatistics_add(NQHttpStatistics*, const char* method, const char* url);
NQ_EXPORT bool NQHttpStatistics_inc(NQHttpStatistics*, const char* method, const char* url);
NQ_EXPORT bool NQHttpStatistics_writeTo(NQHttpStatistics*, NQJSONWriter* writer);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_HTTP_HTTPSTATISTICS_H */
