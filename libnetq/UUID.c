/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/UUID.h"

#include <libnetq/String.h>
#include <libnetq/MinMax.h>
#include <libnetq/CType.h>
#include <libnetq/Assert.h>

#if defined(WITH_OLE32)
# include <combaseapi.h>
#elif defined(NQ_OS_KERNEL)
# include <linux/uuid.h>
#else
# include <libnetq/Random.h>
#endif

static char kURNNIDUUIDString[] = "uuid";
static char kNilUUIDString[] = "00000000-0000-0000-0000-000000000000";
static char kOmniUUIDString[] = "FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF";

static NQUUID kNilUUID = {{
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00,
  0x00, 0x00,
  0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
}};

static NQUUID kOmniUUID = {{
  0xff, 0xff, 0xff, 0xff,
  0xff, 0xff,
  0xff, 0xff,
  0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
}};

void NQUUID_init(NQUUID* uuid, const uint8_t* data, size_t size)
{
  size_t startSize = NQGetMin(sizeof(uuid->data), size);

  if (startSize != 0)
    NQMemcpy(uuid->data, data, startSize);

  size_t lastSize = sizeof(uuid->data) - startSize;

  if (lastSize != 0)
    NQMemset(uuid->data + startSize, 0, lastSize);
}

void NQUUID_initAsCanonical(NQUUID* uuid)
{
#if defined(WITH_OLE32)
  GUID newId;
  CoCreateGuid(&newId);
  uint8_t* data = uuid->data;

  data[0] = (uint8_t)((newId.Data1 >> 24) & 0xFF);
  data[1] = (uint8_t)((newId.Data1 >> 16) & 0xFF);
  data[2] = (uint8_t)((newId.Data1 >> 8) & 0xFF);
  data[3] = (uint8_t)((newId.Data1) & 0xff);

  data[4] = (uint8_t)((newId.Data2 >> 8) & 0xFF);
  data[5] = (uint8_t)((newId.Data2) & 0xff);

  data[6] = (uint8_t)((newId.Data3 >> 8) & 0xFF);
  data[7] = (uint8_t)((newId.Data3) & 0xFF);

  data[8] = (uint8_t)(newId.Data4[0]);
  data[9] = (uint8_t)(newId.Data4[1]);
  data[10] = (uint8_t)(newId.Data4[2]);
  data[11] = (uint8_t)(newId.Data4[3]);
  data[12] = (uint8_t)(newId.Data4[4]);
  data[13] = (uint8_t)(newId.Data4[5]);
  data[14] = (uint8_t)(newId.Data4[6]);
  data[15] = (uint8_t)(newId.Data4[7]);
#elif defined(NQ_OS_KERNEL)
  NQ_STATIC_ASSERT(NQ_UUID_SIZE == UUID_SIZE, "");
  generate_random_uuid(uuid->data);
#else
  NQGetUnlimitedRandom(uuid->data, sizeof(uuid->data));

  /*
  Version:
    1 - The time-based version specified in this document.
    2 - DCE Security version, with embedded POSIX UIDs.
    3 - The name-based version specified in this document that uses MD5 hashing.
    4 - The randomly or pseudo-randomly generated version  specified in this document.
    5 - The name-based version specified in this document that uses SHA-1 hashing.
  */

  int version = 4;
  uuid->data[6] = (uuid->data[6] & 0x0F) | (version << 4); // version
  uuid->data[8] = (uuid->data[8] & 0x3F) | 0x80; // variant
#endif
}

void NQUUID_initAsNil(NQUUID* uuid)
{
  NQMemcpy(uuid->data, kNilUUID.data, sizeof(kNilUUID.data));
}

void NQUUID_initAsOmni(NQUUID* uuid)
{
  NQMemcpy(uuid->data, kOmniUUID.data, sizeof(kOmniUUID.data));
}

static inline uint8_t toASCIIHexValue1(char character)
{
  NQ_ASSERT(NQIsHexDigit(character));
  return character < 'A' ? character - '0' : (character - 'A' + 10) & 0xF;
}

static inline uint8_t toASCIIHexValue(char firstCharacter, char secondCharacter)
{
  return toASCIIHexValue1(firstCharacter) << 4 | toASCIIHexValue1(secondCharacter);
}

static inline bool NQUUID_parseImpl(NQUUID* uuid, const char* s)
{
  size_t i;

  for (i = 0; i < NQ_UUID_STRING_SIZE; i++) {
    char character = s[i];
    if (i == 8 || i == 13 || i == 18 || i == 23) {
      if (character != NQ_UUID_SEPARATOR)
        return false;
    }
    else {
      if (!NQIsHexDigit(character))
        return false;
    }
  }
  
  uuid->data[0] = toASCIIHexValue(s[0], s[1]);
  uuid->data[1] = toASCIIHexValue(s[2], s[3]);
  uuid->data[2] = toASCIIHexValue(s[4], s[5]);
  uuid->data[3] = toASCIIHexValue(s[6], s[7]);

  uuid->data[4] = toASCIIHexValue(s[9], s[10]);
  uuid->data[5] = toASCIIHexValue(s[11], s[12]);

  uuid->data[6] = toASCIIHexValue(s[14], s[15]);
  uuid->data[7] = toASCIIHexValue(s[16], s[17]);

  uuid->data[8] = toASCIIHexValue(s[19], s[20]);
  uuid->data[9] = toASCIIHexValue(s[21], s[22]);

  uuid->data[10] = toASCIIHexValue(s[24], s[25]);
  uuid->data[11] = toASCIIHexValue(s[26], s[27]);
  uuid->data[12] = toASCIIHexValue(s[28], s[29]);
  uuid->data[13] = toASCIIHexValue(s[30], s[31]);
  uuid->data[14] = toASCIIHexValue(s[32], s[33]);
  uuid->data[15] = toASCIIHexValue(s[34], s[35]);

  return true;
}

bool NQUUID_parse(NQUUID* uuid, const char* str)
{
  if (!NQUUID_parseImpl(uuid, str))
    return false;
  return (str[NQ_UUID_STRING_SIZE] == NQ_NIL) ? true : false;
}

bool NQUUID_parse2(NQUUID* uuid, const char* str, size_t len)
{
  if (len != NQ_UUID_STRING_SIZE)
    return false;
  return NQUUID_parseImpl(uuid, str);
}

bool NQUUID_isNil(const NQUUID* uuid)
{
  return (NQMemcmp(uuid, kNilUUID.data, sizeof(kNilUUID.data)) == 0) ? true : false;
}

bool NQUUID_isOmni(const NQUUID* uuid)
{
  return (NQMemcmp(uuid, kOmniUUID.data, sizeof(kOmniUUID.data)) == 0) ? true : false;
}

bool NQUUID_isEqual(const NQUUID* a, const NQUUID* b)
{
  return (NQMemcmp(a, b, sizeof(*b)) == 0) ? true : false;
}

static int NQUUID_sprintf(const NQUUID* uuid, bool isUpper, char* buffer, size_t size)
{
  size_t i;
  const char* hexDigit;

  if (!isUpper)
    hexDigit = "0123456789abcdef";
  else
    hexDigit = "0123456789ABCDEF";

  char* start = buffer;
  char* end = start + size;
  for (i = 0; i < sizeof(uuid->data); i++) {
    if (i == 4 || i == 6 || i == 8 || i == 10) {
      if (end <= start)
        break;

      *start++ = NQ_UUID_SEPARATOR;
    }

    uint8_t b = uuid->data[i];

    if (end <= start)
      break;

    *start++ = hexDigit[(b >> 4) & 0xF];

    if (end <= start)
      break;

    *start++ = hexDigit[b & 0xF];
  }

  if (start < end)
    *start = '\0';

  return NQ_UUID_STRING_SIZE;
}

int NQUUID_sprintfLower(const NQUUID* uuid, char* buffer, size_t size)
{
  return NQUUID_sprintf(uuid, false, buffer, size);
}

int NQUUID_sprintfUpper(const NQUUID* uuid, char* buffer, size_t size)
{
  return NQUUID_sprintf(uuid, true, buffer, size);
}

int NQUUID_sprintfURN(const NQUUID* uuid, char* buffer, size_t size)
{
  // urn:uuid:123e4567-e89b-12d3-a456-426655440000
  static const char prefix[] = "urn:uuid:";
  static const int prefixLength = sizeof(prefix) - 1;

  if (size < prefixLength) {
    if (size != 0)
      NQMemcpy(buffer, prefix, size);
  }
  else {
    NQMemcpy(buffer, prefix, prefixLength);
    buffer += prefixLength;
    size -= prefixLength;

    if (size != 0)
      NQUUID_sprintfLower(uuid, buffer, size);
  }
  
  return prefixLength + NQ_UUID_STRING_SIZE;
}

const NQUUID* NQGetNilUUID(void)
{
  return &kNilUUID;
}

const NQUUID* NQGetOmniUUID(void)
{
  return &kOmniUUID;
}

const char* NQGetNilUUIDString(void)
{
  return kNilUUIDString;
}

const char* NQGetOmniUUIDString(void)
{
  return kOmniUUIDString;
}

const char* NQGetURNNIDUUIDString(void)
{
  return kURNNIDUUIDString;
}

int NQGenerateUUIDStringLower(char* buffer, size_t size)
{
  NQUUID uuid;
  NQUUID_initAsCanonical(&uuid);
  return NQUUID_sprintfLower(&uuid, buffer, size);
}

int NQGenerateUUIDStringUpper(char* buffer, size_t size)
{
  NQUUID uuid;
  NQUUID_initAsCanonical(&uuid);
  return NQUUID_sprintfUpper(&uuid, buffer, size);
}

bool NQIsUUIDString(const char* str)
{
  NQUUID uuid;
  return NQUUID_parse(&uuid, str);
}
