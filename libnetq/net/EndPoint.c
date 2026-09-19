/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/Network.h"

#include <libnetq/string/String.h>
#include <libnetq/string/Sprintf.h>
#include <libnetq/string/Strtox.h>
#include <libnetq/Limits.h>

static const NQIPv4Address s_ip4AddressAny = {{ 0, 0, 0, 0 }};
static const NQIPv4Address s_ip4AddressLoopback = {{ 127, 0, 0, 1 }};
static const NQIPv4Address s_ip4AddressBroadcast = {{ 255, 255, 255, 255 }};

void NQIPv4Address_init(NQIPv4Address* ip4, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
{
  ip4->data[0] = b1;
  ip4->data[1] = b2;
  ip4->data[2] = b3;
  ip4->data[3] = b4;
}

int NQIPv4Address_sprintf(const NQIPv4Address* ip4, char* s, size_t n)
{
  return snprintf(s, n, "%i.%i.%i.%i", ip4->data[0], ip4->data[1], ip4->data[2], ip4->data[3]);
}

static bool parseIPDigits(const char* start, const char* end, const char** tokenEnd, uint8_t* result)
{
  if (end <= start)
    return false;

  while (start < end && *start == ' ')
    start++;

  unsigned number = 0;
  size_t count = 0;
  for (;;) {
    if (end <= start)
      break;

    char c = *start;

    if (c < '0' || '9' < c)
      break;

    number = number * 10 + (c - '0');

    if (number > NQ_UINT8_MAX)
      return false;

    start++;
    count++;
  }

  if (count == 0)
    return false;

  *result = (uint8_t)number;
  *tokenEnd = start;
  return true;
}

bool NQIPv4Address_parse(NQIPv4Address* ip4, const char* s)
{
  return NQIPv4Address_parse2(ip4, s, strlen(s));
}

bool NQIPv4Address_parse2(NQIPv4Address* ip4, const char* s, size_t n)
{
  size_t index;
  uint8_t bytes[4];
  const char* start = s;
  const char* end = start + n;

  for (index = 0; index < sizeof(bytes); index++) {
    if (!parseIPDigits(start, end, &start, bytes + index))
      return false;

    while (start < end && *start == ' ')
      start++;

    if (index < sizeof(bytes) - 1) {
      if (end <= start || *start != '.')
        return false;
      start++;
    }
  }

  if (start < end)
    return false;

  ip4->data[0] = bytes[0];
  ip4->data[1] = bytes[1];
  ip4->data[2] = bytes[2];
  ip4->data[3] = bytes[3];

  return true;
}

bool NQIPv4Address_isBroadcast(const NQIPv4Address* ip4)
{
  return !memcmp(ip4, &s_ip4AddressBroadcast, sizeof(*ip4));
}

bool NQIPv4Address_isAny(const NQIPv4Address* ip4)
{
  return !memcmp(ip4, &s_ip4AddressAny, sizeof(*ip4));
}

bool NQIPv4Address_isUnicast(const NQIPv4Address* ip4)
{
  return ip4->data[3] == 255;
}

const NQIPv4Address* NQIPv4Address_any(void)
{
  return &s_ip4AddressAny;
}

const NQIPv4Address* NQIPv4Address_loopback(void)
{
  return &s_ip4AddressLoopback;
}

const NQIPv4Address* NQIPv4Address_broadcast(void)
{
  return &s_ip4AddressBroadcast;
}

void NQIPv4EndPoint_init(NQIPv4EndPoint* ep4, const NQIPv4Address* address, uint16_t port)
{
  ep4->port = port;
  memcpy(&ep4->address, address, sizeof(*address));
}

int NQIPv4EndPoint_sprintf(const NQIPv4EndPoint* ep4, char* s, size_t n)
{
  return snprintf(s, n, "%i.%i.%i.%i:%i", ep4->address.data[0], ep4->address.data[1], ep4->address.data[2], ep4->address.data[3], ep4->port);
}

bool NQIPv4EndPoint_parse(NQIPv4EndPoint* ep4, const char* s)
{
  return NQIPv4EndPoint_parse2(ep4, s, strlen(s));
}

bool NQIPv4EndPoint_parse2(NQIPv4EndPoint* ep4, const char* s, size_t n)
{
  size_t pos;
  for (pos = 0; pos < n; pos++) {
    if (s[pos] == ':')
      break;
  }

  if (pos == n)
    return false;

  char* end;
  unsigned long port = NQSimpleStrtoul(s + pos + 1, &end, 10);
  if (end != (s + n) || port > NQ_UINT16_MAX)
    return false;

  if (!NQIPv4Address_parse2(&ep4->address, s, pos))
    return false;

  ep4->port = (uint16_t)port;
  return true;
}

void NQEndPoint_init4(NQEndPoint* ep, const uint8_t data[4], uint16_t port)
{
  ep->family = NQ_AF_INET;
  ep->ip4ep.port = port;
  memcpy(ep->ip4ep.address.data, data, sizeof(ep->ip4ep.address.data));
}

void NQEndPoint_init6(NQEndPoint* ep, const uint8_t data[16], uint16_t port)
{
  ep->family = NQ_AF_INET6;
  ep->ip6ep.port = port;
  memcpy(ep->ip6ep.address.data, data, sizeof(ep->ip6ep.address.data));
}

bool NQEndPoint_parse(NQEndPoint* ep, const char* s)
{
  return NQEndPoint_parse2(ep, s, strlen(s));
}

bool NQEndPoint_parse2(NQEndPoint* ep, const char* s, size_t len)
{
  if (NQIPv4EndPoint_parse2(&ep->ip4ep, s, len)) {
    ep->family = NQ_AF_INET;
    return true;
  }
  return false;
}

int NQEndPoint_sprintf(const NQEndPoint* ep, char* s, size_t n)
{
  if (ep->family == NQ_AF_INET)
    return NQIPv4EndPoint_sprintf(&ep->ip4ep, s, n);

  if (s != NULL && n != 0)
    *s = '\0';

  return 0;
}

void NQMACAddress_reset(NQMACAddress* mac)
{
  memset(mac, 0, sizeof(*mac));
}

static bool parseMACAddress(const char* s, size_t n, const char* format, NQMACAddress* result)
{
  const char* start = s;
  const char* end = s + n;

  size_t index = 0;
  NQMACAddress mac;
  for (;;) {
    char fchar = *format++;

    if (fchar == '\0')
      break;

    if (end <= start)
      return false;

    char c = *start++;
    if (fchar == 'X') {
      uint8_t number;
      if ('0' <= c && c <= '9')
        number = c - '0';
      else if ('a' <= c && c <= 'f')
        number = 10 + c - 'a';
      else if ('A' <= c && c <= 'F')
        number = 10 + c - 'A';
      else return false;

      if (index % 2)
        mac.data[index++ / 2] |= number;
      else
        mac.data[index++ / 2] = number << 4;
    }
    else if (fchar != c)
      return false;
  }
  if (start < end)
    return false;

  memcpy(result, &mac, sizeof(mac));
  return true;
}

bool NQMACAddress_parse2(NQMACAddress* mac, const char* s, size_t n)
{
  return parseMACAddress(s, n, "XX:XX:XX:XX:XX:XX", mac)
    || parseMACAddress(s, n, "XX-XX-XX-XX-XX-XX", mac);
}

int NQMACAddress_sprintf(const NQMACAddress* mac, char* s, size_t n)
{
  return snprintf(s, n, "%02X-%02X-%02X-%02X-%02X-%02X", mac->data[0], mac->data[1], mac->data[2], mac->data[3], mac->data[4], mac->data[5]);
}
