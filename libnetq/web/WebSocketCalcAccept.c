/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/web/WebSocketCalcAccept.h"

#include <libnetq/String.h>
#include <libnetq/Base64.h>
#include <libnetq/crypto/SHA1.h>
#include <libnetq/web/WebSocketTypes.h>

int NQWebSocketCalcAccept(const char* key, char* buf, size_t len)
{
  NQSHA1 sha1;
  NQSHA1Digest digest;

  if (!NQSHA1_init(&sha1))
    return -1;

  if (!NQSHA1_update(&sha1, (const uint8_t*)key, strlen(key)))
    return -1;

  if (!NQSHA1_update(&sha1, (const uint8_t*)NQ_WEBSOCKET_SEC_MAGIC, NQ_CSTR_LENGTH(NQ_WEBSOCKET_SEC_MAGIC)))
    return -1;

  if (!NQSHA1_final(&sha1, digest))
    return -1;

  return NQBase64Encode(digest, sizeof(digest), buf, len);
}
