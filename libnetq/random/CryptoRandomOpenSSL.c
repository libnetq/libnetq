/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/random/CryptoRandom.h"

#ifdef NQCONFIG_USE_OPENSSL_CRYPTORAND

#include <openssl/rand.h>
#include <openssl/err.h>

#include <libnetq/ErrorCode.h>
#include <libnetq/Log.h>

int NQGetCryptoRandom(void* data, size_t size)
{
  int rc = RAND_bytes(data, (int)size);
  if (rc != 1) {
    NQ_LOGE("RAND_bytes returned error %lu", ERR_get_error());
    return -NQ_EIO;
  }
  return 0;
}

#endif
