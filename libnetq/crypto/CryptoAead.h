/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_CRYPTO_CRYPTOAEAD_H
#define _LIBNETQ_CRYPTO_CRYPTOAEAD_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_CRYPTOAEAD_KEYSIZE   32
#define NQ_CRYPTOAEAD_NONCESIZE 12
#define NQ_CRYPTOAEAD_TAGSIZE   16

#define NQ_CRYPTOAEAD_SEALSIZE(size)  (NQ_CRYPTOAEAD_NONCESIZE + (size) + NQ_CRYPTOAEAD_TAGSIZE)
#define NQ_CRYPTOAEAD_OPENSIZE(size)  ((size) - NQ_CRYPTOAEAD_NONCESIZE - NQ_CRYPTOAEAD_TAGSIZE)

NQ_EXPORT int NQCryptoAeadSeal(const void* key, const void* data, size_t size, void* result);
NQ_EXPORT int NQCryptoAeadOpen(const void* key, const void* data, size_t size, void* result);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LIBNETQ_CRYPTO_CRYPTOAEAD_H */
