/*
 * MIT License
 *
 * Copyright (c) 2023-2025  Yurii Yakubin (yurii.yakubin@gmail.com)
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

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_GETCOMPUTERNAME_H */
