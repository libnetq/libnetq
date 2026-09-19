/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_HTTP_HTTPBUILDER_H
#define _LIBNETQ_HTTP_HTTPBUILDER_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQHttpBuilder NQHttpBuilder;
struct NQHttpBuilder {
  uint16_t size;
  uint8_t data[1024 - sizeof(uint16_t)];
};

NQ_EXPORT bool NQHttpBuilder_init(NQHttpBuilder*); // TODO: ? Pass buffer
NQ_EXPORT void NQHttpBuilder_finalize(NQHttpBuilder*);

NQ_EXPORT bool NQHttpBuilder_addRequest(NQHttpBuilder*, const char* method, const char* url, const char* version);
NQ_EXPORT bool NQHttpBuilder_addGetRequest(NQHttpBuilder*, const char* url);
NQ_EXPORT bool NQHttpBuilder_addStatusLine(NQHttpBuilder*, const char* version, const char* code, const char* reason);
NQ_EXPORT bool NQHttpBuilder_addStatusCode(NQHttpBuilder*, uint16_t code);
NQ_EXPORT bool NQHttpBuilder_addHeader(NQHttpBuilder*, const char* name, const char* value);
NQ_EXPORT bool NQHttpBuilder_finish(NQHttpBuilder*);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_HTTP_HTTPBUILDER_H */
