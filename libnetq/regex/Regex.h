/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_REGEX_REGEX_H
#define _LIBNETQ_REGEX_REGEX_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_REGEX_CASELESS  (1 << 0)
#define NQ_REGEX_MULTILINE (1 << 1)
#define NQ_REGEX_UTF       (1 << 2)

typedef struct NQRegex NQRegex;
typedef struct NQRegexMatch NQRegexMatch;

NQ_EXPORT const char* NQRegex_package(void);
NQ_EXPORT const char* NQRegex_version(void);

NQ_EXPORT NQRegex* NQRegex_create(const char* pattern, int flags);
NQ_EXPORT NQRegex* NQRegex_create2(const char* pattern, size_t length, int flags);
NQ_EXPORT void NQRegex_release(NQRegex*);

NQ_EXPORT NQRegexMatch* NQRegexMatch_create(NQRegex* regex);
NQ_EXPORT NQRegexMatch* NQRegexMatch_create2(size_t size);
NQ_EXPORT void NQRegexMatch_release(NQRegexMatch*);
NQ_EXPORT size_t NQRegexMatch_count(const NQRegexMatch*);
NQ_EXPORT bool NQRegexMatch_substrAt(const NQRegexMatch*, size_t index, size_t* offset, size_t* length);

NQ_EXPORT bool NQRegex_exec(NQRegex*, const char* str, NQRegexMatch* match, int flags);
NQ_EXPORT bool NQRegex_exec2(NQRegex*, const char* str, size_t len, NQRegexMatch* match, int flags);
NQ_EXPORT int NQRegex_indexOf(const NQRegex*, const char* name);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_REGEX_REGEX_H */
