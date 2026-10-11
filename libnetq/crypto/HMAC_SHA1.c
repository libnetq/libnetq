/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/crypto/HMAC_SHA1.h"

#include <libnetq/String.h>
#include <libnetq/Log.h>

#define HMAC_IPAD_VALUE 0x36
#define HMAC_OPAD_VALUE 0x5c

static inline bool initImpl(NQHMAC_SHA1* thiz, const uint8_t* key, size_t length)
{
  uint8_t rkey[64];
  if (length <= sizeof(rkey)) {
    NQMemcpy(rkey, key, length);
    NQMemset(rkey + length, 0, sizeof(rkey) - length);
  }
  else {
    NQSHA1 sha1;
    if (!NQSHA1_init(&sha1))
      return false;
    if (!NQSHA1_update(&sha1, key, length)) {
      NQSHA1_finalize(&sha1);
      return false;
    }
    uint8_t digest[NQSHA1_DIGEST_SIZE];
    if (!NQSHA1_final(&sha1, digest)) {
      NQSHA1_finalize(&sha1);
      return false;
    }
    NQSHA1_finalize(&sha1);
    NQMemcpy(rkey, digest, sizeof(digest));
    NQMemset(rkey + sizeof(digest), 0, sizeof(rkey) - sizeof(digest));
  }

  for(size_t i = 0; i < sizeof(rkey); i++)
    rkey[i] ^= HMAC_OPAD_VALUE;

  if (!NQSHA1_update(&thiz->opad, rkey, sizeof(rkey)))
    return false;

  for(size_t i = 0; i < sizeof(rkey); i++)
    rkey[i] ^= HMAC_OPAD_VALUE ^ HMAC_IPAD_VALUE;

  return NQSHA1_update(&thiz->ipad, rkey, sizeof(rkey));
}

bool NQHMAC_SHA1_init(NQHMAC_SHA1* thiz, const uint8_t* key, size_t length)
{
  if (!NQSHA1_init(&thiz->opad)) {
    return false;
  }
  if (!NQSHA1_init(&thiz->ipad)) {
    NQSHA1_finalize(&thiz->opad);
    return false;
  }
  if (!initImpl(thiz, key, length)) {
    NQSHA1_finalize(&thiz->opad);
    NQSHA1_finalize(&thiz->ipad);
    return false;
  }
  return true;
}

bool NQHMAC_SHA1_final(NQHMAC_SHA1* thiz, uint8_t digest[NQSHA1_DIGEST_SIZE])
{
  uint8_t temp[NQSHA1_DIGEST_SIZE];
  NQSHA1_final(&thiz->ipad, temp);
  NQSHA1_update(&thiz->opad, temp, sizeof(temp));
  return NQSHA1_final(&thiz->opad, digest);
}
