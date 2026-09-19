/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/crypto/Secp256k1.h"

#ifdef NQCONFIG_USE_STUB_SECP256K1

NQSecp256k1Ctx* NQSecp256k1Create()
{
  return NULL;
}

void NQSecp256k1Destroy(NQSecp256k1Ctx* ctx)
{
  NQ_UNUSED_PARAM(ctx);
}

bool NQSecp256k1SeckeyGenerate(NQSecp256k1Ctx* ctx, uint8_t* seckey)
{
  NQ_UNUSED_PARAM(ctx);
  NQ_UNUSED_PARAM(seckey);
  return false;
}

NQSecp256k1Pubkey* NQSecp256k1PubkeyCreate(const NQSecp256k1Ctx* ctx, const uint8_t* seckey)
{
  NQ_UNUSED_PARAM(ctx);
  NQ_UNUSED_PARAM(seckey);
  return NULL;
}

NQSecp256k1Pubkey* NQSecp256k1PubkeyParse(const NQSecp256k1Ctx* ctx, const uint8_t* data, size_t size)
{
  NQ_UNUSED_PARAM(ctx);
  NQ_UNUSED_PARAM(data);
  NQ_UNUSED_PARAM(size);
  return NULL;
}

void NQSecp256k1PubkeyDestroy(const NQSecp256k1Ctx* ctx, NQSecp256k1Pubkey* pubkey)
{
  NQ_UNUSED_PARAM(ctx);
  NQ_UNUSED_PARAM(pubkey);
}

bool NQSecp256k1PubkeySerialize(const NQSecp256k1Ctx* ctx, const NQSecp256k1Pubkey* pubkey, uint8_t* data, size_t size)
{
  NQ_UNUSED_PARAM(ctx);
  NQ_UNUSED_PARAM(pubkey);
  NQ_UNUSED_PARAM(data);
  NQ_UNUSED_PARAM(size);
  return false;
}

bool NQSecp256k1EcdhPerform(const NQSecp256k1Ctx* ctx, const NQSecp256k1Pubkey* pubkey, const uint8_t* seckey, uint8_t* output32)
{
  NQ_UNUSED_PARAM(ctx);
  NQ_UNUSED_PARAM(pubkey);
  NQ_UNUSED_PARAM(seckey);
  NQ_UNUSED_PARAM(output32);
  return false;
}

NQSecp256k1Signature* NQSecp256k1SignatureCreate(const NQSecp256k1Ctx* ctx, const uint8_t* seckey, const uint8_t* hash32)
{
  NQ_UNUSED_PARAM(ctx);
  NQ_UNUSED_PARAM(seckey);
  NQ_UNUSED_PARAM(hash32);
  return NULL;
}

NQSecp256k1Signature* NQSecp256k1SignatureParse(const NQSecp256k1Ctx* ctx, const uint8_t* input64)
{
  NQ_UNUSED_PARAM(ctx);
  NQ_UNUSED_PARAM(input64);
  return NULL;
}

void NQSecp256k1SignatureDestroy(const NQSecp256k1Ctx* ctx, NQSecp256k1Signature* sig)
{
  NQ_UNUSED_PARAM(ctx);
  NQ_UNUSED_PARAM(sig);
}

bool NQSecp256k1SignatureSerialize(const NQSecp256k1Ctx* ctx, const NQSecp256k1Signature* sig, uint8_t* output64)
{
  NQ_UNUSED_PARAM(ctx);
  NQ_UNUSED_PARAM(sig);
  NQ_UNUSED_PARAM(output64);
  return false;
}

bool NQSecp256k1SignatureVerify(const NQSecp256k1Ctx* ctx, const NQSecp256k1Signature* sig, const NQSecp256k1Pubkey* pubkey, const uint8_t* hash32)
{
  NQ_UNUSED_PARAM(ctx);
  NQ_UNUSED_PARAM(sig);
  NQ_UNUSED_PARAM(pubkey);
  NQ_UNUSED_PARAM(hash32);
  return false;
}

bool NQSecp256k1EllswiftMake(const NQSecp256k1Ctx* ctx, const uint8_t* seckey, const uint8_t* auxrnd32, uint8_t* ellspkey)
{
  NQ_UNUSED_PARAM(ctx);
  NQ_UNUSED_PARAM(seckey);
  NQ_UNUSED_PARAM(auxrnd32);
  NQ_UNUSED_PARAM(ellspkey);
  return false;
}

bool NQSecp256k1EllswiftXdhA(const NQSecp256k1Ctx* ctx, const uint8_t* ellspkeyA, const uint8_t* ellspkeyB, const uint8_t* seckey, uint8_t* output32)
{
  NQ_UNUSED_PARAM(ctx);
  NQ_UNUSED_PARAM(ellspkeyA);
  NQ_UNUSED_PARAM(ellspkeyB);
  NQ_UNUSED_PARAM(seckey);
  NQ_UNUSED_PARAM(output32);
  return false;
}

bool NQSecp256k1EllswiftXdhB(const NQSecp256k1Ctx* ctx, const uint8_t* ellspkeyA, const uint8_t* ellspkeyB, const uint8_t* seckey, uint8_t* output32)
{
  NQ_UNUSED_PARAM(ctx);
  NQ_UNUSED_PARAM(ellspkeyA);
  NQ_UNUSED_PARAM(ellspkeyB);
  NQ_UNUSED_PARAM(seckey);
  NQ_UNUSED_PARAM(output32);
  return false;
}

#endif
