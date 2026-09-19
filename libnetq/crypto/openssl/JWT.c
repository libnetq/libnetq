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

#ifdef NQCONFIG_USE_OPENSSL_JWT

#include <openssl/opensslv.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/err.h>

#include <libnetq/string/String.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/ByteBuffer.h>
#include <libnetq/Malloc.h>
#include <libnetq/Base64.h>
#include <libnetq/Assert.h>
#include <libnetq/Log.h>

struct HMACStruct {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
  HMAC_CTX ctx;
#else
  HMAC_CTX* ctx;
#endif
};

static inline bool HMAC_init(struct HMACStruct* hmac)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
  HMAC_CTX_init(&hmac->ctx);
  return true;
#else
  hmac->ctx = HMAC_CTX_new();
  return hmac->ctx != NULL;
#endif
}

static inline void HMAC_finalize(struct HMACStruct* hmac)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
  HMAC_CTX_cleanup(&hmac->ctx);
#else
  HMAC_CTX_free(hmac->ctx);
  hmac->ctx = NULL;
#endif
}

static inline HMAC_CTX* HMAC_impl(struct HMACStruct* hmac)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
  return &hmac->ctx;
#else
  return hmac->ctx;
#endif
}

struct NQJWT {
  NQJSON* header;
  NQJSON* claims;
  NQByteBuffer token;
  const EVP_MD* md;
  struct HMACStruct hmac;
  uint8_t digest[1];
};

static bool alg2md(const char* alg, const EVP_MD** md)
{
  if (NQStrcmp(alg, NQ_JWT_ALG_NONE) == 0)
    *md = NULL;
  else if (NQStrcmp(alg, NQ_JWT_ALG_HS256) == 0)
    *md = EVP_sha256();
  else if (NQStrcmp(alg, NQ_JWT_ALG_HS384) == 0)
    *md = EVP_sha384();
  else if (NQStrcmp(alg, NQ_JWT_ALG_HS512) == 0)
    *md = EVP_sha512();
  else {
    NQ_LOGE("Unknown crypt algorithm (%s)", alg);
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
  HMAC_CTX* ctx = HMAC_impl(&thiz->hmac);
  if (HMAC_Init_ex(ctx, seckey, sklen, thiz->md, NULL) != 1) {
    NQ_LOGE("%s (%lu)", ERR_error_string(ERR_get_error(), NULL), ERR_get_error());
    return false;
  }

  if (HMAC_Update(ctx, data, size) != 1) {
    NQ_LOGE("%s (%lu)", ERR_error_string(ERR_get_error(), NULL), ERR_get_error());
    return false;
  }

  if (HMAC_Final(ctx, thiz->digest, NULL) != 1) {
    NQ_LOGE("%s (%lu)", ERR_error_string(ERR_get_error(), NULL), ERR_get_error());
    return false;
  }

  return true;
}

static NQJWT* createInternal(NQJSON* header, NQJSON* claims, const EVP_MD* md)
{
  size_t digestSize = md ? EVP_MD_size(md) : 0;
  NQJWT* thiz = (NQJWT*)NQMalloc(sizeof(*thiz) + digestSize - sizeof(thiz->digest));
  if (thiz == NULL)
    return NULL;

  if (md != NULL && !HMAC_init(&thiz->hmac)) {
    NQFree(thiz);
    return NULL;
  }

  thiz->md = md;
  thiz->header = header;
  thiz->claims = claims;
  NQByteBuffer_init(&thiz->token);

  return thiz;
}

NQJWT* NQJWT_create(const char* alg)
{
  const EVP_MD* md;
  if (!alg2md(alg, &md)) {
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

  NQJWT* thiz = createInternal(header, claims, md);
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
  const EVP_MD* md;
  if (!alg2md(alg, &md)) {
    NQJSON_release(header);
    return NULL;
  }

  if (md == NULL && info.signature.length != 0) {
    NQ_LOGE("JWT algorithm 'none' but signature present");
    NQJSON_release(header);
    return NULL;
  }

  if (md != NULL && info.signature.length == 0) {
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

  NQJWT* thiz = createInternal(header, claims, md);
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

  if (md != NULL) {
    size_t digestSize = EVP_MD_size(md);
    NQ_ASSERT(digestSize < tokenLength);

    uint8_t* digest = NQByteBuffer_data(&thiz->token);
    int sz = NQBase64DecodeEx(info.signature.characters, info.signature.length, digest, digestSize, NQ_BASE64_URL | NQ_BASE64_NONPAD);
    if (sz != digestSize) {
      NQJWT_release(thiz);
      return NULL;
    }

    size_t signingSize = NQJWTTokenInfo_signingSize(&info);
    if (!updateDigest(thiz, (const uint8_t*)token, signingSize, seckey, sklen) || NQMemcmp(thiz->digest, digest, digestSize) != 0) {
      NQJWT_release(thiz);
      return NULL;
    }
  }

  NQMemcpy(NQByteBuffer_data(&thiz->token), token, tokenLength);
  return thiz;
}

void NQJWT_release(NQJWT* thiz)
{
  if (thiz->md != NULL) {
    HMAC_finalize(&thiz->hmac);
  }

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
  size_t b64Size = NQBase64EncodeLength(size);

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
  if (!addDataAsBase64ToToken(thiz, thiz->digest, EVP_MD_size(thiz->md)))
    return false;
  return true;
}

bool NQJWT_sign(NQJWT* thiz, const void* seckey, size_t sklen)
{
  NQByteBuffer_resize(&thiz->token, 0);

  if (thiz->md == NULL) {
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
    NQMemcpy(buffer, NQByteBuffer_data(&thiz->token), length);
  else {
    NQMemcpy(buffer, NQByteBuffer_data(&thiz->token), result);
    buffer[result] = '\0';
  }

  return (int)result;
}

#endif
