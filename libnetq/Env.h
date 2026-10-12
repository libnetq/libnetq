/*
 * MIT License
 *
 * Copyright (c) 2020-2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_ENV_H
#define _LIBNETQ_ENV_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

NQ_EXPORT const char* NQEnvCreate(const char* name);
NQ_EXPORT void NQEnvFree(const char* value);

NQ_EXPORT int NQEnvGet(const char* name, char* value, size_t n); // NQEnvRead
NQ_EXPORT int NQEnvSet(const char* name, const char* value);
NQ_EXPORT int NQEnvUnset(const char* name);

typedef struct NQEnviron NQEnviron;
typedef struct NQEnviron NQEnvironIter;

NQ_EXPORT NQEnviron* NQEnviron_create(void);
NQ_EXPORT void NQEnviron_destroy(NQEnviron*);

NQ_EXPORT const NQEnvironIter* NQEnviron_begin(const NQEnviron*);
NQ_EXPORT const NQEnvironIter* NQEnvironIter_next(const NQEnvironIter*);
NQ_EXPORT size_t NQEnvironIter_read(const NQEnvironIter*, char* buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_ENV_H */
