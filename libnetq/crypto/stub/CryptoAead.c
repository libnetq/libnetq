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

#ifdef CRYPTOAEAD_BACKEND_STUB

#include <libnetq/ErrorCode.h>

int NQCryptoAeadSeal(const void* key, const void* data, size_t size, void* result)
{
  NQ_UNUSED_PARAM(key);
  NQ_UNUSED_PARAM(data);
  NQ_UNUSED_PARAM(size);
  NQ_UNUSED_PARAM(result);
  return -NQ_ENOTSUPP;
}

int NQCryptoAeadOpen(const void* key, const void* data, size_t size, void* result)
{
  NQ_UNUSED_PARAM(key);
  NQ_UNUSED_PARAM(data);
  NQ_UNUSED_PARAM(size);
  NQ_UNUSED_PARAM(result);
  return -NQ_ENOTSUPP;
}

#endif /* CRYPTOAEAD_BACKEND_STUB */
