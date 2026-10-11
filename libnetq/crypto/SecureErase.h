/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_CRYPTO_SECUREERASE_H
#define _LIBNETQ_CRYPTO_SECUREERASE_H

#include <libnetq/Basic.h>

#ifdef NQ_OS_WINDOWS
#include <windows.h>
#else
#include <libnetq/string/String.h>
#include <libnetq/Atomic.h>
#endif

static inline void NQSecureErase(void* data, size_t size)
{
#ifdef NQ_OS_WINDOWS
  SecureZeroMemory(data, size);
#else
  NQMemset(data, 0, size);
  NQCompilerFence();
#endif
}

#endif /* _LIBNETQ_CRYPTO_SECUREERASE_H */
