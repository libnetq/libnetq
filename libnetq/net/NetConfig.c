/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"

#if defined(_WIN32)
# include <winsock2.h>
# include <ws2tcpip.h>
#endif

#include "libnetq/net/NetConfig.h"

#include <libnetq/ErrorCode.h>
#include <libnetq/Malloc.h>
#include <libnetq/string/String.h>

#if defined(NQ_OS_UNIX)
# include <netinet/in.h>
# include <sys/socket.h>
# include <unistd.h>
#endif

void NQNetFreeLinks(NQNetLink* links, size_t count)
{
  (void)count;
  NQFree(links);
}

void NQNetFreeAddresses(NQNetAddress* addresses, size_t count)
{
  (void)count;
  NQFree(addresses);
}

void NQNetFreeRoutes(NQNetRoute* routes, size_t count)
{
  if (routes == NULL)
    return;
  for (size_t i = 0; i < count; i++)
    NQFree(routes[i].nexthops);
  NQFree(routes);
}

void NQNetFreeNeighbors(NQNetNeighbor* neighbors, size_t count)
{
  (void)count;
  NQFree(neighbors);
}

#if !defined(NQ_OS_LINUX)

/* Whether the first `bits` bits of a and b are the same */
static bool prefix_match(const uint8_t* a, const uint8_t* b, unsigned bits)
{
  unsigned bytes = bits / 8;
  if (bytes && NQMemcmp(a, b, bytes) != 0)
    return false;
  bits %= 8;
  if (bits == 0)
    return true;
  uint8_t mask = (uint8_t)(0xFF << (8 - bits));
  return ((a[bytes] ^ b[bytes]) & mask) == 0;
}

/* The source address the system picks for `dst`: a connected UDP socket
 * sends nothing but knows it */
static bool source_for(int family, const uint8_t* dst, uint8_t* src)
{
  struct sockaddr_storage ss;
  socklen_t len;
  NQMemset(&ss, 0, sizeof(ss));
  if (family == NQ_NET_INET) {
    struct sockaddr_in* sin = (struct sockaddr_in*)&ss;
    sin->sin_family = AF_INET;
    sin->sin_port = htons(9);
    NQMemcpy(&sin->sin_addr, dst, 4);
    len = sizeof(*sin);
  } else {
    struct sockaddr_in6* sin6 = (struct sockaddr_in6*)&ss;
    sin6->sin6_family = AF_INET6;
    sin6->sin6_port = htons(9);
    NQMemcpy(&sin6->sin6_addr, dst, 16);
    len = sizeof(*sin6);
  }

# if defined(NQ_OS_WINDOWS)
  SOCKET s = socket(ss.ss_family, SOCK_DGRAM, IPPROTO_UDP);
  if (s == INVALID_SOCKET)
    return false;
# else
  int s = socket(ss.ss_family, SOCK_DGRAM, IPPROTO_UDP);
  if (s < 0)
    return false;
# endif
  bool ok = connect(s, (struct sockaddr*)&ss, len) == 0;
  if (ok) {
    len = sizeof(ss);
    ok = getsockname(s, (struct sockaddr*)&ss, &len) == 0;
  }
# if defined(NQ_OS_WINDOWS)
  closesocket(s);
# else
  close(s);
# endif
  if (!ok)
    return false;
  if (family == NQ_NET_INET)
    NQMemcpy(src, &((struct sockaddr_in*)&ss)->sin_addr, 4);
  else
    NQMemcpy(src, &((struct sockaddr_in6*)&ss)->sin6_addr, 16);
  return true;
}

/* Without a kernel to ask: the longest matching prefix of the main and
 * local tables, with the lowest metric among equals */
int NQNetRouteGet(const NQNetRouteQuery* query, NQNetRoute** route)
{
  *route = NULL;
  int family = query->family ? query->family : NQ_NET_INET;
  unsigned bits = family == NQ_NET_INET ? 32 : 128;

  NQNetRoute* routes;
  size_t count;
  int ret = NQNetGetRoutes(family, &routes, &count);
  if (ret != 0)
    return ret;

  NQNetRoute* best = NULL;
  for (size_t i = 0; i < count; i++) {
    NQNetRoute* r = &routes[i];
    if (r->type == NQ_NET_RTN_BROADCAST || r->type == NQ_NET_RTN_MULTICAST)
      continue;
    if ((query->have & NQ_NET_QUERY_OIF) && r->oif != query->oif)
      continue;
    if (r->dstLength && !prefix_match(r->dst, query->dst, r->dstLength))
      continue;
    if (best == NULL || r->dstLength > best->dstLength ||
        (r->dstLength == best->dstLength && r->priority < best->priority))
      best = r;
  }
  if (best == NULL) {
    NQNetFreeRoutes(routes, count);
    return -NQ_ESRCH;
  }

  NQNetRoute* found = (NQNetRoute*)NQMalloc(sizeof(NQNetRoute));
  if (found == NULL) {
    NQNetFreeRoutes(routes, count);
    return -NQ_ENOMEM;
  }
  *found = *best;
  found->nexthops = NULL;
  found->nexthopCount = 0;
  NQNetFreeRoutes(routes, count);

  /* described the way Linux describes a lookup: the host, what it goes
   * through and the source address */
  int local = found->type == NQ_NET_RTN_LOCAL;
  found->type = local ? NQ_NET_RTN_LOCAL : NQ_NET_RTN_UNICAST;
  found->flags |= NQ_NET_RTM_F_CLONED;
  found->dstLength = (uint8_t)bits;
  NQMemcpy(found->dst, query->dst, bits / 8);
  found->have |= NQ_NET_ROUTE_DST;
  found->have &= ~(NQ_NET_ROUTE_PRIORITY | NQ_NET_ROUTE_METRICS);
  found->table = NQ_NET_TABLE_MAIN;
  if (query->have & NQ_NET_QUERY_SRC) {
    NQMemcpy(found->prefsrc, query->src, bits / 8);
    found->have |= NQ_NET_ROUTE_PREFSRC;
  } else if (source_for(family, query->dst, found->prefsrc)) {
    found->have |= NQ_NET_ROUTE_PREFSRC;
  }
  if (local)
    found->flags |= 0x80000000u; /* RTCF_LOCAL */
  *route = found;
  return 0;
}

#endif /* !NQ_OS_LINUX */

#if !defined(NQ_OS_LINUX) && !defined(NQ_OS_DARWIN) && !defined(NQ_OS_WINDOWS)

int NQNetGetLinks(NQNetLink** links, size_t* count)
{
  *links = NULL;
  *count = 0;
  return -NQ_ENOTSUPP;
}

int NQNetGetAddresses(int family, NQNetAddress** addresses, size_t* count)
{
  (void)family;
  *addresses = NULL;
  *count = 0;
  return -NQ_ENOTSUPP;
}

int NQNetGetRoutes(int family, NQNetRoute** routes, size_t* count)
{
  (void)family;
  *routes = NULL;
  *count = 0;
  return -NQ_ENOTSUPP;
}

int NQNetGetNeighbors(int family, NQNetNeighbor** neighbors, size_t* count)
{
  (void)family;
  *neighbors = NULL;
  *count = 0;
  return -NQ_ENOTSUPP;
}

#endif

#if !defined(NQ_OS_LINUX)

const char* NQNetGetErrorMessage(void)
{
  return "";
}

int NQNetLinkChange(NQNetChange change, const NQNetLinkRequest* request)
{
  (void)change;
  (void)request;
  return -NQ_ENOTSUPP;
}

int NQNetAddressChange(NQNetChange change, const NQNetAddress* address)
{
  (void)change;
  (void)address;
  return -NQ_ENOTSUPP;
}

int NQNetRouteChange(NQNetChange change, const NQNetRoute* route)
{
  (void)change;
  (void)route;
  return -NQ_ENOTSUPP;
}

int NQNetNeighborChange(NQNetChange change, const NQNetNeighbor* neighbor)
{
  (void)change;
  (void)neighbor;
  return -NQ_ENOTSUPP;
}

#endif
