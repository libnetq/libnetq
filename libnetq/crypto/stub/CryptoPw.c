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

#ifdef CRYPTOPW_BACKEND_STUB

bool NQCryptoPwHash(const char* password, const void* salt, void* hash)
{
  NQ_UNUSED_PARAM(password);
  NQ_UNUSED_PARAM(salt);
  NQ_UNUSED_PARAM(hash);
  return false;
}

bool NQCryptoPwVerify(const char* password, const void* salt, const void* hash)
{
  NQ_UNUSED_PARAM(password);
  NQ_UNUSED_PARAM(salt);
  NQ_UNUSED_PARAM(hash);
  return false;
}

#endif /* CRYPTOPW_BACKEND_STUB */
