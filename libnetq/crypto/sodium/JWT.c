/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/crypto/JWT.h"

#ifdef NQCONFIG_USE_LIBSODIUM_JWT

#include <sodium.h>

#include <libnetq/string/String.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/ByteBuffer.h>
#include <libnetq/Malloc.h>
#include <libnetq/Base64.h>
#include <libnetq/Assert.h>
#include <libnetq/Log.h>

enum JWTAlg {
  ALG_NONE,
  ALG_HS256,
  ALG_HS512,
};

struct NQJWT {
  NQJSON* header;
  NQJSON* claims;
  NQByteBuffer token;
  enum JWTAlg alg;
  uint8_t digest[crypto_auth_hmacsha512_BYTES];
};

static inline size_t digestSize(enum JWTAlg alg)
{
  switch (alg) {
  case ALG_HS256:
    return crypto_auth_hmacsha256_BYTES;
  case ALG_HS512:
    return crypto_auth_hmacsha512_BYTES;
  default:
    return 0;
  }
}

static bool str2alg(const char* str, enum JWTAlg* alg)
{
  if (NQStrcmp(str, NQ_JWT_ALG_NONE) == 0)
    *alg = ALG_NONE;
  else if (NQStrcmp(str, NQ_JWT_ALG_HS256) == 0)
    *alg = ALG_HS256;
  else if (NQStrcmp(str, NQ_JWT_ALG_HS512) == 0)
    *alg = ALG_HS512;
  else if (NQStrcmp(str, NQ_JWT_ALG_HS384) == 0) {
    NQ_LOGE("Crypt algorithm (%s) is not supported by libsodium", str);
    return false;
  }
  else {
    NQ_LOGE("Unknown crypt algorithm (%s)", str);
    return false;
  }
  return true;
}

static NQJSON* encodeJSON(const char* str, size_t len)
{
  NQJSON* result = NULL;

  NQStringPrint buffer;
  NQStringPrint_init(&buffer);

  if (NQStringPrint_resize(&buffer, len)) {
    char* characters = NQStringPrint_characters(&buffer);
    size_t length = NQStringPrint_length(&buffer);
    int sz = NQBase64DecodeEx(str, len, characters, length, NQ_BASE64_URL | NQ_BASE64_NONPAD);
    if (sz > 0)
      result = NQJSON_parse2(characters, (size_t)sz);
  }

  NQStringPrint_finalize(&buffer);
  return result;
}

static bool updateDigest(NQJWT* thiz, const uint8_t* data, size_t size, const void* seckey, size_t sklen)
{
  switch (thiz->alg) {
  case ALG_HS256: {
    crypto_auth_hmacsha256_state state;
    if (crypto_auth_hmacsha256_init(&state, (const unsigned char*)seckey, sklen) != 0 ||
        crypto_auth_hmacsha256_update(&state, data, size) != 0 ||
        crypto_auth_hmacsha256_final(&state, thiz->digest) != 0) {
      sodium_memzero(&state, sizeof(state));
      NQ_LOGE("HMAC-SHA256 failed");
      return false;
    }
    sodium_memzero(&state, sizeof(state));
    return true;
  }
  case ALG_HS512: {
    crypto_auth_hmacsha512_state state;
    if (crypto_auth_hmacsha512_init(&state, (const unsigned char*)seckey, sklen) != 0 ||
        crypto_auth_hmacsha512_update(&state, data, size) != 0 ||
        crypto_auth_hmacsha512_final(&state, thiz->digest) != 0) {
      sodium_memzero(&state, sizeof(state));
      NQ_LOGE("HMAC-SHA512 failed");
      return false;
    }
    sodium_memzero(&state, sizeof(state));
    return true;
  }
  default:
    NQ_ASSERT(false);
    return false;
  }
}

static NQJWT* createInternal(NQJSON* header, NQJSON* claims, enum JWTAlg alg)
{
  if (sodium_init() < 0) {
    NQ_LOGE("Failed to initialize libsodium");
    return NULL;
  }

  NQJWT* thiz = (NQJWT*)NQMalloc(sizeof(*thiz));
  if (thiz == NULL)
    return NULL;

  thiz->alg = alg;
  thiz->header = header;
  thiz->claims = claims;
  NQByteBuffer_init(&thiz->token);

  return thiz;
}

NQJWT* NQJWT_create(const char* alg)
{
  enum JWTAlg jwtAlg;
  if (!str2alg(alg, &jwtAlg)) {
    return NULL;
  }

  NQJSON* header = NQJSON_createObjectRef();
  if (header == NULL) {
    return NULL;
  }

  if (!NQJSON_objectSetString(header, NQ_JWT_HDR_ALG, alg)) {
    NQJSON_release(header);
    return NULL;
  }

  if (!NQJSON_objectSetString(header, NQ_JWT_HDR_TYP, NQ_JWT_TYP_JWT)) {
    NQJSON_release(header);
    return NULL;
  }

  NQJSON* claims = NQJSON_createObjectRef();
  if (claims == NULL) {
    NQJSON_release(header);
    return NULL;
  }

  NQJWT* thiz = createInternal(header, claims, jwtAlg);
  if (thiz == NULL) {
    NQJSON_release(header);
    NQJSON_release(claims);
    return NULL;
  }

  return thiz;
}

NQJWT* NQJWT_parse(const char* token, const void* seckey, size_t sklen)
{
  struct NQJWTTokenInfo info;
  if (!NQJWTTokenInfoParse(token, &info)) {
    NQ_LOGE("Failed to parse JWT token");
    return NULL;
  }

  NQJSON* header = encodeJSON(info.header.characters, info.header.length);
  if (header == NULL) {
    NQ_LOGE("Failed to decode JWT header");
    return NULL;
  }

  NQJSON* typeJson = NQJSON_objectGet(header, NQ_JWT_HDR_TYP);
  if (!NQJSON_isString(typeJson)) {
    NQ_LOGE("JWT header missing '%s' string", NQ_JWT_HDR_TYP);
    NQJSON_release(header);
    return NULL;
  }

  if (NQStrcmp(NQJSON_asString(typeJson), NQ_JWT_TYP_JWT) != 0) {
    NQ_LOGE("Unsupported JWT type '%s'", NQJSON_asString(typeJson));
    NQJSON_release(header);
    return NULL;
  }

  NQJSON* algJson = NQJSON_objectGet(header, NQ_JWT_HDR_ALG);
  if (!NQJSON_isString(algJson)) {
    NQ_LOGE("JWT header missing '%s' string", NQ_JWT_HDR_ALG);
    NQJSON_release(header);
    return NULL;
  }

  const char* alg = NQJSON_asString(algJson);
  enum JWTAlg jwtAlg;
  if (!str2alg(alg, &jwtAlg)) {
    NQJSON_release(header);
    return NULL;
  }

  if (jwtAlg == ALG_NONE && info.signature.length != 0) {
    NQ_LOGE("JWT algorithm 'none' but signature present");
    NQJSON_release(header);
    return NULL;
  }

  if (jwtAlg != ALG_NONE && info.signature.length == 0) {
    NQ_LOGE("JWT algorithm '%s' requires a signature", alg);
    NQJSON_release(header);
    return NULL;
  }

  NQJSON* claims = encodeJSON(info.payload.characters, info.payload.length);
  if (claims == NULL) {
    NQ_LOGE("Failed to decode JWT payload");
    NQJSON_release(header);
    return NULL;
  }

  NQJWT* thiz = createInternal(header, claims, jwtAlg);
  if (thiz == NULL) {
    NQ_LOGE("No Memory");
    NQJSON_release(header);
    NQJSON_release(claims);
    return NULL;
  }

  size_t tokenLength = NQJWTTokenInfo_tokenSize(&info);

  if (!NQByteBuffer_resize(&thiz->token, tokenLength)) {
    NQ_LOGE("No Memory");
    NQJWT_release(thiz);
    return NULL;
  }

  if (jwtAlg != ALG_NONE) {
    size_t dsize = digestSize(jwtAlg);
    NQ_ASSERT(dsize < tokenLength);

    uint8_t* digest = NQByteBuffer_data(&thiz->token);
    int sz = NQBase64DecodeEx(info.signature.characters, info.signature.length, digest, dsize, NQ_BASE64_URL | NQ_BASE64_NONPAD);
    if (sz != (int)dsize) {
      NQJWT_release(thiz);
      return NULL;
    }

    size_t signingSize = NQJWTTokenInfo_signingSize(&info);
    if (!updateDigest(thiz, (const uint8_t*)token, signingSize, seckey, sklen) || sodium_memcmp(thiz->digest, digest, dsize) != 0) {
      NQJWT_release(thiz);
      return NULL;
    }
  }

  memcpy(NQByteBuffer_data(&thiz->token), token, tokenLength);
  return thiz;
}

void NQJWT_release(NQJWT* thiz)
{
  sodium_memzero(thiz->digest, sizeof(thiz->digest));
  NQByteBuffer_finalize(&thiz->token);
  NQJSON_release(thiz->header);
  NQJSON_release(thiz->claims);
  NQFree(thiz);
}

bool NQJWT_headerGetBool(NQJWT* thiz, const char* name, bool* value)
{
  return NQJSON_objectGetBool(thiz->header, name, value);
}

bool NQJWT_headerGetInt64(NQJWT* thiz, const char* name, int64_t* value)
{
  return NQJSON_objectGetInt64(thiz->header, name, value);
}

bool NQJWT_headerGetString(NQJWT* thiz, const char* name, const char** value)
{
  return NQJSON_objectGetString(thiz->header, name, value);
}

bool NQJWT_headerSetBool(NQJWT* thiz, const char* name, bool value)
{
  if (NQJWTHeaderIsReserved(name))
    return false;
  return NQJSON_objectSetBool(thiz->header, name, value);
}

bool NQJWT_headerSetInt64(NQJWT* thiz, const char* name, int64_t value)
{
  if (NQJWTHeaderIsReserved(name))
    return false;
  return NQJSON_objectSetInt64(thiz->header, name, value);
}

bool NQJWT_headerSetString(NQJWT* thiz, const char* name, const char* value)
{
  if (NQJWTHeaderIsReserved(name))
    return false;
  return NQJSON_objectSetString(thiz->header, name, value);
}

bool NQJWT_claimGetBool(NQJWT* thiz, const char* name, bool* value)
{
  return NQJSON_objectGetBool(thiz->claims, name, value);
}

bool NQJWT_claimGetInt64(NQJWT* thiz, const char* name, int64_t* value)
{
  return NQJSON_objectGetInt64(thiz->claims, name, value);
}

bool NQJWT_claimGetString(NQJWT* thiz, const char* name, const char** value)
{
  return NQJSON_objectGetString(thiz->claims, name, value);
}

bool NQJWT_claimSetBool(NQJWT* thiz, const char* name, bool value)
{
  return NQJSON_objectSetBool(thiz->claims, name, value);
}

bool NQJWT_claimSetInt64(NQJWT* thiz, const char* name, int64_t value)
{
  return NQJSON_objectSetInt64(thiz->claims, name, value);
}

bool NQJWT_claimSetString(NQJWT* thiz, const char* name, const char* value)
{
  return NQJSON_objectSetString(thiz->claims, name, value);
}

static bool addDataAsBase64ToToken(NQJWT* thiz, const uint8_t* data, size_t size)
{
  size_t b64Size = ((size + 2) / 3) * 4;

  size_t oldb64Size = NQByteBuffer_size(&thiz->token);
  if (!NQByteBuffer_resize(&thiz->token, oldb64Size + b64Size)) {
    return false;
  }

  char* b64Data = (char*)NQByteBuffer_data(&thiz->token) + oldb64Size;
  int result = NQBase64EncodeEx(data, size, b64Data, b64Size, NQ_BASE64_URL | NQ_BASE64_NONPAD);
  NQ_ASSERT(result <= b64Size);
  if (result < 0) {
    NQByteBuffer_resize(&thiz->token, oldb64Size);
    return false;
  }

  if (result < b64Size) {
    NQByteBuffer_resize(&thiz->token, oldb64Size + (size_t)result);
  }

  return true;
}

static bool addJSONAsBase64ToToken(NQJWT* thiz, const NQJSON* json)
{
  NQStringPrint buffer;
  NQStringPrint_init(&buffer);

  bool success = NQJSON_dump(json, &buffer);
  if (success) {
    const char* jsonStr = NQStringPrint_characters(&buffer);
    size_t jsonLen = NQStringPrint_length(&buffer);
    success = addDataAsBase64ToToken(thiz, (uint8_t*)jsonStr, jsonLen);
  }

  NQStringPrint_finalize(&buffer);
  return success;
}

static bool buildToken(NQJWT* thiz)
{
  static const uint8_t s_dot = '.';
  NQ_ASSERT(NQByteBuffer_isEmpty(&thiz->token));
  if (!addJSONAsBase64ToToken(thiz, thiz->header))
    return false;
  if (!NQByteBuffer_append(&thiz->token, &s_dot, 1))
    return false;
  if (!addJSONAsBase64ToToken(thiz, thiz->claims))
    return false;
  if (!NQByteBuffer_append(&thiz->token, &s_dot, 1))
    return false;
  return true;
}

static bool signToken(NQJWT* thiz, const void* seckey, size_t sklen)
{
  NQ_ASSERT(!NQByteBuffer_isEmpty(&thiz->token));
  if (!updateDigest(thiz, NQByteBuffer_data(&thiz->token), NQByteBuffer_size(&thiz->token) - 1, seckey, sklen))
    return false;
  if (!addDataAsBase64ToToken(thiz, thiz->digest, digestSize(thiz->alg)))
    return false;
  return true;
}

bool NQJWT_sign(NQJWT* thiz, const void* seckey, size_t sklen)
{
  NQByteBuffer_resize(&thiz->token, 0);

  if (thiz->alg == ALG_NONE) {
    if (sklen != 0) {
      NQ_LOGE("JWT algorithm 'none' does not accept a secret key");
      return false;
    }
    if (!buildToken(thiz)) {
      NQByteBuffer_resize(&thiz->token, 0);
      return false;
    }
  }
  else {
    if (sklen == 0) {
      NQ_LOGE("JWT signing key is missing");
      return false;
    }
    if (!buildToken(thiz) || !signToken(thiz, seckey, sklen)) {
      NQByteBuffer_resize(&thiz->token, 0);
      return false;
    }
  }

  return true;
}

int NQJWT_token(NQJWT* thiz, char* buffer, size_t length)
{
  if (NQByteBuffer_isEmpty(&thiz->token))
    return -NQ_ENOENT;

  size_t result = NQByteBuffer_size(&thiz->token);
  if (length < (result + 1))
    memcpy(buffer, NQByteBuffer_data(&thiz->token), length);
  else {
    memcpy(buffer, NQByteBuffer_data(&thiz->token), result);
    buffer[result] = '\0';
  }

  return (int)result;
}

#endif
