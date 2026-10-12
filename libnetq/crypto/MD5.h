/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_CRYPTO_MD5_H
#define _LIBNETQ_CRYPTO_MD5_H

#include <libnetq/Basic.h>

#if defined(NQCONFIG_USE_OPENSSL_MD5)
# include <openssl/md5.h>
#elif defined(NQCONFIG_USE_GENERIC_MD5)
#else
# include <libnetq/crypto/CryptoHash.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define NQMD5_DIGEST_SIZE 16
typedef uint8_t NQMD5Digest[NQMD5_DIGEST_SIZE];

#if defined(NQCONFIG_USE_OPENSSL_MD5) || defined(NQCONFIG_USE_GENERIC_MD5)

#ifdef NQCONFIG_USE_OPENSSL_MD5
#define NQMD5 MD5_CTX
#else
typedef struct NQMD5 NQMD5;
struct NQMD5 {
  uint32_t state[4];
  uint64_t length;
  uint8_t buffer[64];
};
#endif

NQ_EXPORT bool NQMD5_init(NQMD5*);
static inline void NQMD5_finalize(NQMD5* thiz)
{
  NQ_UNUSED_PARAM(thiz);
}

NQ_EXPORT bool NQMD5_update(NQMD5*, const uint8_t* data, size_t size);
NQ_EXPORT bool NQMD5_final(NQMD5*, uint8_t digest[NQMD5_DIGEST_SIZE]);

#else

typedef struct NQCryptoHash NQMD5;

static inline bool NQMD5_init(NQMD5* thiz)
{
  // TODO: Check digestSize
  return NQCryptoHash_init(thiz, NQ_CRYPTOALGORITHM_MD5);
}

static inline void NQMD5_finalize(NQMD5* thiz)
{
  NQCryptoHash_finalize(thiz);
}

static inline bool NQMD5_update(NQMD5* thiz, const uint8_t* data, size_t size)
{
  return NQCryptoHash_update(thiz, data, size);
}

static inline bool NQMD5_final(NQMD5* thiz, uint8_t* digest)
{
  return NQCryptoHash_final(thiz, digest);
}

#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LIBNETQ_CRYPTO_MD5_H */
