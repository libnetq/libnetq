/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_GETCOMPUTERNAME_H
#define _LIBNETQ_GETCOMPUTERNAME_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

NQ_EXPORT int NQGetComputerName(char* buffer, size_t n);
NQ_EXPORT int NQSetComputerName(const char* name);

NQ_EXPORT int NQGetNisDomainName(char* buffer, size_t n);
NQ_EXPORT int NQSetNisDomainName(const char* name);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_GETCOMPUTERNAME_H */
