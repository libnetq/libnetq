/*
 * MIT License
 *
 * Copyright (c) 2022-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_STRING_CSTRTONUM_H
#define _LIBNETQ_STRING_CSTRTONUM_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

NQ_EXPORT int32_t NQCStrToInt32(const char* str, char** end, int base);
NQ_EXPORT uint32_t NQCStrToUint32(const char* str, char** end, int base);

NQ_EXPORT int64_t NQCStrToInt64(const char* str, char** end, int base);
NQ_EXPORT uint64_t NQCStrToUint64(const char* str, char** end, int base);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_STRING_CSTRTONUM_H */
