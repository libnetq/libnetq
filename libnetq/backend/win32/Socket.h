/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_BACKEND_WIN32_SOCKET_H
#define _LIBNETQ_BACKEND_WIN32_SOCKET_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NQ_OS_WINDOWS
NQ_EXPORT void NQWinSocketInitialize(void);
NQ_EXPORT void NQWinSocketCleanup(void);
#else
static inline void NQWinSocketInitialize(void) {}
static inline void NQWinSocketCleanup(void) {}
#endif

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_BACKEND_WIN32_SOCKET_H */
