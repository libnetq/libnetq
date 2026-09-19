/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/crypto/CryptoPw.h"

#ifdef CRYPTOPW_BACKEND_OPENSSL

#include <openssl/evp.h>

#include <libnetq/string/String.h>
#include <libnetq/crypto/SecureErase.h>

#define NQ_BCRYPT_ITER 100000

static inline bool cryptoPwHash(const char* password, const void* salt, void* hash)
{
  size_t len = NQStrlen(password);
  return PKCS5_PBKDF2_HMAC(password, (int)len, salt, NQ_CRYPTOPW_SALTSIZE,
      NQ_BCRYPT_ITER, EVP_sha256(), NQ_CRYPTOPW_HASHSIZE, hash) == 1;
}

bool NQCryptoPwHash(const char* password, const void* salt, void* hash)
{
  return cryptoPwHash(password, salt, hash);
}

bool NQCryptoPwVerify(const char* password, const void* salt, const void* hash)
{
  uint8_t buffer[NQ_CRYPTOPW_HASHSIZE];
  bool result = cryptoPwHash(password, salt, buffer) && CRYPTO_memcmp(hash, buffer, NQ_CRYPTOPW_HASHSIZE) == 0;
  NQSecureErase(buffer, sizeof(buffer));
  return result;
}

#endif /* CRYPTOPW_BACKEND_OPENSSL */
