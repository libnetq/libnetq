/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/web/UrlHost.h"

#include <libnetq/Limits.h>
#include <libnetq/String.h>
#include <libnetq/string/Strtox.h>
#include <libnetq/Network.h>
#include <libnetq/Malloc.h>

struct NQUrlHost {
  const char* characters;
  uint16_t length;
  uint16_t flags;
  uint16_t port;
  uint16_t wwwIndex;
  uint16_t wwwLength;
  uint16_t hostIndex;
  uint16_t hostLength;
  uint16_t portIndex;
  uint16_t portLength;
  NQIPv4Address ipv4Addr;
};

#define HAS_IPV4ADDR (1 << 0)
#define HAS_PORT (1 << 1)

static bool NQUrlHost_parse(NQUrlHost* thiz, const char* characters)
{
  const char* end = characters;
  thiz->flags = 0;

  for (thiz->length = 0; *end != '\0'; end++) {
    if (++thiz->length == 0) // overflow
      return false;
  }

  thiz->wwwIndex = 0;
  if (thiz->length > 4 && memcmp(characters, "www.", 4) == 0) {
    thiz->wwwLength = 3;
    thiz->hostIndex = 4;
  }
  else {
    thiz->wwwLength = 0;
    thiz->hostIndex = 0;
  }

  const char* hostBegin = characters + thiz->hostIndex;
  const char* hostEnd = strstr(hostBegin, ":");
  if (hostEnd != NULL) {
    const char* portBegin = hostEnd + 1;

    char* portEnd;
    unsigned long num = NQSimpleStrtoul(portBegin, &portEnd, 10);
    if (*portEnd != '\0' || NQ_UINT16_MAX < num)
      return false;

    thiz->hostLength = (uint16_t)(hostEnd - hostBegin);
    thiz->portIndex = (uint16_t)(portBegin - characters);
    thiz->portLength = (uint16_t)(end - portBegin);
    thiz->port = (uint16_t)num;
  }
  else {
    thiz->hostLength = (uint16_t)(end - hostBegin);
    thiz->portIndex = 0;
    thiz->portLength = 0;
    thiz->port = 0;
    thiz->flags |= HAS_PORT;
  }

  if (NQIPv4Address_parse2(&thiz->ipv4Addr, characters + thiz->hostIndex, thiz->hostLength)) {
    thiz->flags |= HAS_IPV4ADDR;
  }

  thiz->characters = characters;

  return true;
}

NQUrlHost* NQUrlHost_create(const char* characters)
{
  NQUrlHost priv;
  if (!NQUrlHost_parse(&priv, characters))
    return NULL;

  size_t lenz = priv.length + 1;
  NQUrlHost* thiz;
  thiz = (NQUrlHost*)NQMalloc(sizeof(*thiz) + lenz);
  if (thiz == NULL)
    return NULL;

  char* str = (char*)thiz + sizeof(*thiz);
  memcpy(str, characters, lenz);
  thiz->characters = str;

  thiz->length = priv.length;
  thiz->flags = priv.flags;
  thiz->port = priv.port;
  thiz->wwwIndex = priv.wwwIndex;
  thiz->wwwLength = priv.wwwLength;
  thiz->hostIndex = priv.hostIndex;
  thiz->hostLength = priv.hostLength;
  thiz->portIndex = priv.portIndex;
  thiz->portLength = priv.portLength;

  memcpy(&thiz->ipv4Addr, &priv.ipv4Addr, sizeof(priv.ipv4Addr));

  return thiz;
}

void NQUrlHost_destroy(NQUrlHost* thiz)
{
  NQFree(thiz);
}

const char* NQUrlHost_characters(const NQUrlHost* thiz)
{
  return thiz->characters;
}

uint16_t NQUrlHost_port(const NQUrlHost* thiz)
{
  return thiz->port;
}

void NQUrlHost_setPort(NQUrlHost* thiz, uint16_t port)
{
  thiz->flags &= ~HAS_PORT;
  thiz->port = port;
}

bool NQUrlHost_hasPort(const NQUrlHost* thiz)
{
  return (thiz->flags & HAS_PORT) == 0;
}

bool NQUrlHost_equal(const NQUrlHost* thiz, const NQUrlHost* other)
{
  if (NQUrlHost_hasPort(thiz) != NQUrlHost_hasPort(other))
    return false;

  if (thiz->port != other->port)
    return false;

  if (thiz->hostLength != other->hostLength)
    return false;

  if (memcmp(thiz->characters + thiz->hostIndex, other->characters + other->hostIndex, thiz->hostLength) != 0)
    return false;

  return true;
}

bool NQUrlHost_equalCharacters(const NQUrlHost* thiz, const char* characters)
{
  NQUrlHost other;
  if (!NQUrlHost_parse(&other, characters))
    return false;

  return NQUrlHost_equal(thiz, &other);
}

bool NQUrlHost_isIPv4Address(const NQUrlHost* thiz)
{
  return (thiz->flags & HAS_IPV4ADDR) ? true : false;
}
