/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/crypto/CryptoAead.h"

#ifdef CRYPTOAEAD_BACKEND_OPENSSL

#include <libnetq/Limits.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/random/CryptoRandom.h>
#include <libnetq/crypto/SecureErase.h>

#include <openssl/evp.h>

int NQCryptoAeadSeal(const void* key, const void* data, size_t size, void* result)
{
  if (size > NQ_INT_MAX - NQ_CRYPTOAEAD_NONCESIZE - NQ_CRYPTOAEAD_TAGSIZE)
    return -NQ_EINVAL;

  uint8_t* nonce = (uint8_t*)result;
  uint8_t* ct = nonce + NQ_CRYPTOAEAD_NONCESIZE;
  uint8_t* tag = ct + size;

  int ret = NQGetCryptoRandom(nonce, NQ_CRYPTOAEAD_NONCESIZE);
  if (ret != 0) {
    return ret;
  }

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (ctx == NULL)
    return -NQ_ENOMEM;

  if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, nonce) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return -NQ_EIO;
  }

  int n;
  if (EVP_EncryptUpdate(ctx, ct, &n, data, (int)size) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return -NQ_EIO;
  }

  if (EVP_EncryptFinal_ex(ctx, ct + n, &n) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return -NQ_EIO;
  }

  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, NQ_CRYPTOAEAD_TAGSIZE, tag) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return -NQ_EIO;
  }

  EVP_CIPHER_CTX_free(ctx);
  return NQ_CRYPTOAEAD_NONCESIZE + (int)size + NQ_CRYPTOAEAD_TAGSIZE;
}

int NQCryptoAeadOpen(const void* key, const void* data, size_t size, void* result)
{
  if (size < NQ_CRYPTOAEAD_NONCESIZE + NQ_CRYPTOAEAD_TAGSIZE)
    return -NQ_EOVERFLOW;

  if (size > NQ_INT_MAX)
    return -NQ_EOVERFLOW;

  const uint8_t* nonce = (const uint8_t*)data;
  const uint8_t* ct = nonce + NQ_CRYPTOAEAD_NONCESIZE;
  size_t ctlen = (int)size - NQ_CRYPTOAEAD_NONCESIZE - NQ_CRYPTOAEAD_TAGSIZE;
  const uint8_t* tag = ct + ctlen;

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (ctx == NULL)
    return -NQ_ENOMEM;

  if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, nonce) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return -NQ_EIO;
  }

  int n;
  if (EVP_DecryptUpdate(ctx, result, &n, ct, (int)ctlen) != 1) {
    NQSecureErase(result, ctlen);
    EVP_CIPHER_CTX_free(ctx);
    return -NQ_EIO;
  }

  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, NQ_CRYPTOAEAD_TAGSIZE, (void*)tag) != 1) {
    NQSecureErase(result, ctlen);
    EVP_CIPHER_CTX_free(ctx);
    return -NQ_EIO;
  }

  if (EVP_DecryptFinal_ex(ctx, (uint8_t*)result + n, &n) != 1) {
    NQSecureErase(result, ctlen);
    EVP_CIPHER_CTX_free(ctx);
    return -NQ_EIO;
  }

  EVP_CIPHER_CTX_free(ctx);
  return (int)ctlen;
}

#endif /* CRYPTOAEAD_BACKEND_OPENSSL */
