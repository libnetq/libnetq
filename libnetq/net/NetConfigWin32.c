/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

/* NetConfig.h on Windows: the IP Helper API (Vista and later). Read only.
 * Interfaces are named by their friendly names ("Ethernet 2"). */

#include "config.h"

#if defined(_WIN32)
# include <winsock2.h>
# include <ws2tcpip.h>
# include <iphlpapi.h>
#endif

#include "libnetq/net/NetConfig.h"

#if defined(NQ_OS_WINDOWS)

#include <libnetq/ErrorCode.h>
#include <libnetq/Malloc.h>
#include <libnetq/string/String.h>

/* A growing array of `size`-byte items */
typedef struct {
  void* items;
  size_t count, cap, size;
} List;

static void* list_add(List* l)
{
  if (l->count == l->cap) {
    size_t cap = l->cap ? l->cap * 2 : 16;
    void* p = NQRealloc(l->items, cap * l->size);
    if (p == NULL)
      return NULL;
    l->items = p;
    l->cap = cap;
  }
  void* item = (char*)l->items + l->count++ * l->size;
  NQMemset(item, 0, l->size);
  return item;
}

/* The adapters with their addresses; free with NQFree */
static int get_adapters(int family, IP_ADAPTER_ADDRESSES** out)
{
  ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_INCLUDE_ALL_INTERFACES;
  ULONG af = family == NQ_NET_INET ? AF_INET : family == NQ_NET_INET6 ? AF_INET6 : AF_UNSPEC;
  ULONG size = 16384;
  IP_ADAPTER_ADDRESSES* adapters = NULL;
  ULONG status;
  do {
    NQFree(adapters);
    adapters = (IP_ADAPTER_ADDRESSES*)NQMalloc(size);
    if (adapters == NULL)
      return -NQ_ENOMEM;
    status = GetAdaptersAddresses(af, flags, NULL, adapters, &size);
  } while (status == ERROR_BUFFER_OVERFLOW);
  if (status == ERROR_NO_DATA) {
    NQFree(adapters);
    *out = NULL;
    return 0;
  }
  if (status != NO_ERROR) {
    NQFree(adapters);
    return -(int)status;
  }
  *out = adapters;
  return 0;
}

static unsigned adapter_index(const IP_ADAPTER_ADDRESSES* a)
{
  return a->IfIndex ? a->IfIndex : a->Ipv6IfIndex;
}

static void utf8_name(const wchar_t* name, char* out, size_t size)
{
  int n = WideCharToMultiByte(CP_UTF8, 0, name, -1, out, (int)size, NULL, NULL);
  if (n <= 0)
    out[0] = '\0';
  out[size - 1] = '\0';
}

static unsigned linux_type(IFTYPE type)
{
  switch (type) {
  case IF_TYPE_ETHERNET_CSMACD:
  case IF_TYPE_IEEE80211:
    return NQ_NET_ARPHRD_ETHER;
  case IF_TYPE_SOFTWARE_LOOPBACK:
    return NQ_NET_ARPHRD_LOOPBACK;
  case IF_TYPE_PPP:
    return NQ_NET_ARPHRD_PPP;
  default:
    return NQ_NET_ARPHRD_NONE;
  }
}

static uint8_t linux_operstate(IF_OPER_STATUS status)
{
  switch (status) {
  case IfOperStatusUp: return NQ_NET_OPER_UP;
  case IfOperStatusDown: return NQ_NET_OPER_DOWN;
  case IfOperStatusTesting: return NQ_NET_OPER_TESTING;
  case IfOperStatusDormant: return NQ_NET_OPER_DORMANT;
  case IfOperStatusNotPresent: return NQ_NET_OPER_NOTPRESENT;
  case IfOperStatusLowerLayerDown: return NQ_NET_OPER_LOWERLAYERDOWN;
  default: return NQ_NET_OPER_UNKNOWN;
  }
}

int NQNetGetLinks(NQNetLink** links, size_t* count)
{
  *links = NULL;
  *count = 0;
  IP_ADAPTER_ADDRESSES* adapters;
  int ret = get_adapters(NQ_NET_UNSPEC, &adapters);
  if (ret != 0)
    return ret;

  List list = {NULL, 0, 0, sizeof(NQNetLink)};
  for (IP_ADAPTER_ADDRESSES* a = adapters; a != NULL; a = a->Next) {
    NQNetLink* l = (NQNetLink*)list_add(&list);
    if (l == NULL) {
      NQFree(adapters);
      NQFree(list.items);
      return -NQ_ENOMEM;
    }
    l->index = adapter_index(a);
    utf8_name(a->FriendlyName, l->name, sizeof(l->name));
    l->type = linux_type(a->IfType);
    l->operState = linux_operstate(a->OperStatus);
    l->mtu = a->Mtu;
    l->have = NQ_NET_LINK_MTU | NQ_NET_LINK_OPERSTATE;
    if (a->OperStatus == IfOperStatusUp)
      l->flags |= NQ_NET_IFF_UP | NQ_NET_IFF_RUNNING | NQ_NET_IFF_LOWER_UP;
    if (l->type == NQ_NET_ARPHRD_LOOPBACK)
      l->flags |= NQ_NET_IFF_LOOPBACK;
    else if (l->type == NQ_NET_ARPHRD_ETHER)
      l->flags |= NQ_NET_IFF_BROADCAST | NQ_NET_IFF_MULTICAST;
    else
      l->flags |= NQ_NET_IFF_NOARP;
    if (a->PhysicalAddressLength > 0 && a->PhysicalAddressLength <= NQ_NET_HWADDR_SIZE) {
      l->addressLength = a->PhysicalAddressLength;
      NQMemcpy(l->address, a->PhysicalAddress, a->PhysicalAddressLength);
      if (l->flags & NQ_NET_IFF_BROADCAST) {
        l->broadcastLength = l->addressLength;
        NQMemset(l->broadcast, 0xFF, l->broadcastLength);
      }
    }

    MIB_IF_ROW2 row;
    NQMemset(&row, 0, sizeof(row));
    row.InterfaceIndex = l->index;
    if (GetIfEntry2(&row) == NO_ERROR) {
      l->have |= NQ_NET_LINK_STATS | NQ_NET_LINK_STATS64;
      l->stats.rxBytes = row.InOctets;
      l->stats.rxPackets = row.InUcastPkts + row.InNUcastPkts;
      l->stats.rxErrors = row.InErrors;
      l->stats.rxDropped = row.InDiscards;
      /* MIB_IF_ROW2 has no multicast packet counter (only InMulticastOctets)
         and no promiscuous flag, so stats.multicast stays 0 and IFF_PROMISC
         is never reported on Windows. */
      l->stats.txBytes = row.OutOctets;
      l->stats.txPackets = row.OutUcastPkts + row.OutNUcastPkts;
      l->stats.txErrors = row.OutErrors;
      l->stats.txDropped = row.OutDiscards;
    }
  }
  NQFree(adapters);
  *links = (NQNetLink*)list.items;
  *count = list.count;
  return 0;
}

int NQNetGetAddresses(int family, NQNetAddress** addresses, size_t* count)
{
  *addresses = NULL;
  *count = 0;
  IP_ADAPTER_ADDRESSES* adapters;
  int ret = get_adapters(family, &adapters);
  if (ret != 0)
    return ret;

  List list = {NULL, 0, 0, sizeof(NQNetAddress)};
  for (IP_ADAPTER_ADDRESSES* ad = adapters; ad != NULL; ad = ad->Next) {
    for (IP_ADAPTER_UNICAST_ADDRESS* u = ad->FirstUnicastAddress; u != NULL; u = u->Next) {
      const struct sockaddr* sa = u->Address.lpSockaddr;
      if (sa->sa_family != AF_INET && sa->sa_family != AF_INET6)
        continue;
      NQNetAddress* a = (NQNetAddress*)list_add(&list);
      if (a == NULL) {
        NQFree(adapters);
        NQFree(list.items);
        return -NQ_ENOMEM;
      }
      a->index = adapter_index(ad);
      a->prefixLength = u->OnLinkPrefixLength;
      a->have = NQ_NET_ADDR_LOCAL | NQ_NET_ADDR_ADDRESS | NQ_NET_ADDR_LIFETIME;
      a->validLifetime = u->ValidLifetime;
      a->preferredLifetime = u->PreferredLifetime;
      bool dynamic = u->PrefixOrigin == IpPrefixOriginDhcp || u->PrefixOrigin == IpPrefixOriginRouterAdvertisement ||
                     u->SuffixOrigin == IpSuffixOriginDhcp || u->SuffixOrigin == IpSuffixOriginRandom;
      a->flags = dynamic ? 0 : NQ_NET_IFA_PERMANENT;
      if (u->DadState == IpDadStateTentative)
        a->flags |= NQ_NET_IFA_TENTATIVE;
      else if (u->DadState == IpDadStateDeprecated)
        a->flags |= NQ_NET_IFA_DEPRECATED;
      else if (u->DadState == IpDadStateDuplicate)
        a->flags |= NQ_NET_IFA_DADFAILED;
      if (sa->sa_family == AF_INET) {
        a->family = NQ_NET_INET;
        NQMemcpy(a->local, &((const struct sockaddr_in*)sa)->sin_addr, 4);
        NQMemcpy(a->address, a->local, 4);
        a->scope = a->local[0] == 127 ? NQ_NET_SCOPE_HOST : NQ_NET_SCOPE_UNIVERSE;
        if (a->prefixLength < 31 && ad->IfType != IF_TYPE_SOFTWARE_LOOPBACK) {
          uint32_t host = a->prefixLength ? 0xFFFFFFFFu >> a->prefixLength : 0xFFFFFFFFu;
          for (int i = 0; i < 4; i++)
            a->broadcast[i] = (uint8_t)(a->local[i] | (uint8_t)(host >> (24 - 8 * i)));
          a->have |= NQ_NET_ADDR_BROADCAST;
        }
        utf8_name(ad->FriendlyName, a->label, sizeof(a->label));
        a->have |= NQ_NET_ADDR_LABEL;
      } else {
        a->family = NQ_NET_INET6;
        NQMemcpy(a->local, &((const struct sockaddr_in6*)sa)->sin6_addr, 16);
        NQMemcpy(a->address, a->local, 16);
        static const uint8_t loopback[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
        if (NQMemcmp(a->local, loopback, 16) == 0)
          a->scope = NQ_NET_SCOPE_HOST;
        else if (a->local[0] == 0xfe && (a->local[1] & 0xc0) == 0x80)
          a->scope = NQ_NET_SCOPE_LINK;
        else
          a->scope = NQ_NET_SCOPE_UNIVERSE;
        if (u->SuffixOrigin == IpSuffixOriginRandom)
          a->flags |= NQ_NET_IFA_SECONDARY;
      }
    }
  }
  NQFree(adapters);
  *addresses = (NQNetAddress*)list.items;
  *count = list.count;
  return 0;
}

static bool sockaddr_inet_bytes(const SOCKADDR_INET* sa, int* family, uint8_t* out)
{
  if (sa->si_family == AF_INET) {
    *family = NQ_NET_INET;
    NQMemcpy(out, &sa->Ipv4.sin_addr, 4);
    return true;
  }
  if (sa->si_family == AF_INET6) {
    *family = NQ_NET_INET6;
    NQMemcpy(out, &sa->Ipv6.sin6_addr, 16);
    return true;
  }
  return false;
}

static bool is_zero(const uint8_t* a, size_t n)
{
  for (size_t i = 0; i < n; i++) {
    if (a[i])
      return false;
  }
  return true;
}

static uint8_t linux_protocol(NL_ROUTE_PROTOCOL protocol)
{
  switch (protocol) {
  case MIB_IPPROTO_LOCAL: return NQ_NET_RTPROT_KERNEL;
  case MIB_IPPROTO_NETMGMT:
  case MIB_IPPROTO_NT_STATIC:
  case MIB_IPPROTO_NT_STATIC_NON_DOD: return NQ_NET_RTPROT_STATIC;
  case MIB_IPPROTO_NT_AUTOSTATIC: return NQ_NET_RTPROT_BOOT;
  case MIB_IPPROTO_ICMP: return NQ_NET_RTPROT_REDIRECT;
  default: return NQ_NET_RTPROT_UNSPEC;
  }
}

int NQNetGetRoutes(int family, NQNetRoute** routes, size_t* count)
{
  *routes = NULL;
  *count = 0;
  ADDRESS_FAMILY af = family == NQ_NET_INET ? AF_INET : family == NQ_NET_INET6 ? AF_INET6 : AF_UNSPEC;
  MIB_IPFORWARD_TABLE2* table = NULL;
  DWORD status = GetIpForwardTable2(af, &table);
  if (status != NO_ERROR)
    return status == ERROR_NOT_FOUND ? 0 : -(int)status;

  /* the addresses of this host, for the local routes */
  NQNetAddress* addrs = NULL;
  size_t naddrs = 0;
  NQNetGetAddresses(family, &addrs, &naddrs);

  List list = {NULL, 0, 0, sizeof(NQNetRoute)};
  for (ULONG i = 0; i < table->NumEntries; i++) {
    const MIB_IPFORWARD_ROW2* row = &table->Table[i];
    NQNetRoute* r = (NQNetRoute*)list_add(&list);
    if (r == NULL) {
      FreeMibTable(table);
      NQFree(addrs);
      NQFree(list.items);
      return -NQ_ENOMEM;
    }
    int f;
    if (!sockaddr_inet_bytes(&row->DestinationPrefix.Prefix, &f, r->dst)) {
      list.count--;
      continue;
    }
    size_t bytes = f == NQ_NET_INET ? 4 : 16;
    r->family = f;
    r->dstLength = row->DestinationPrefix.PrefixLength;
    if (r->dstLength)
      r->have |= NQ_NET_ROUTE_DST;
    r->table = NQ_NET_TABLE_MAIN;
    r->type = NQ_NET_RTN_UNICAST;
    r->protocol = linux_protocol(row->Protocol);
    r->oif = row->InterfaceIndex;
    r->priority = row->Metric;
    r->have |= NQ_NET_ROUTE_OIF | NQ_NET_ROUTE_PRIORITY;
    int gf;
    if (sockaddr_inet_bytes(&row->NextHop, &gf, r->gateway) && !is_zero(r->gateway, bytes)) {
      r->have |= NQ_NET_ROUTE_GATEWAY;
      r->scope = NQ_NET_SCOPE_UNIVERSE;
    } else {
      r->scope = f == NQ_NET_INET ? NQ_NET_SCOPE_LINK : NQ_NET_SCOPE_UNIVERSE;
    }

    /* where Linux keeps them: the local table */
    bool multicast = f == NQ_NET_INET ? (r->dst[0] & 0xF0) == 0xE0 : r->dst[0] == 0xFF;
    if (multicast) {
      r->type = NQ_NET_RTN_MULTICAST;
      r->table = NQ_NET_TABLE_LOCAL;
    } else if (r->dstLength == bytes * 8) {
      for (size_t k = 0; k < naddrs; k++) {
        if (addrs[k].family == f && NQMemcmp(addrs[k].local, r->dst, bytes) == 0) {
          r->type = NQ_NET_RTN_LOCAL;
          r->scope = NQ_NET_SCOPE_HOST;
          break;
        }
        if (f == NQ_NET_INET && (addrs[k].have & NQ_NET_ADDR_BROADCAST) && NQMemcmp(addrs[k].broadcast, r->dst, 4) == 0) {
          r->type = NQ_NET_RTN_BROADCAST;
          break;
        }
      }
      if (f == NQ_NET_INET && r->dst[0] == 255 && r->dst[1] == 255 && r->dst[2] == 255 && r->dst[3] == 255)
        r->type = NQ_NET_RTN_BROADCAST;
      if (r->type != NQ_NET_RTN_UNICAST)
        r->table = NQ_NET_TABLE_LOCAL;
    }
  }
  FreeMibTable(table);
  NQFree(addrs);
  *routes = (NQNetRoute*)list.items;
  *count = list.count;
  return 0;
}

static uint16_t linux_neigh_state(NL_NEIGHBOR_STATE state)
{
  switch (state) {
  case NlnsUnreachable: return NQ_NET_NUD_FAILED;
  case NlnsIncomplete: return NQ_NET_NUD_INCOMPLETE;
  case NlnsProbe: return NQ_NET_NUD_PROBE;
  case NlnsDelay: return NQ_NET_NUD_DELAY;
  case NlnsStale: return NQ_NET_NUD_STALE;
  case NlnsReachable: return NQ_NET_NUD_REACHABLE;
  case NlnsPermanent: return NQ_NET_NUD_PERMANENT;
  default: return NQ_NET_NUD_NONE;
  }
}

int NQNetGetNeighbors(int family, NQNetNeighbor** neighbors, size_t* count)
{
  *neighbors = NULL;
  *count = 0;
  ADDRESS_FAMILY af = family == NQ_NET_INET ? AF_INET : family == NQ_NET_INET6 ? AF_INET6 : AF_UNSPEC;
  MIB_IPNET_TABLE2* table = NULL;
  DWORD status = GetIpNetTable2(af, &table);
  if (status != NO_ERROR)
    return status == ERROR_NOT_FOUND ? 0 : -(int)status;

  List list = {NULL, 0, 0, sizeof(NQNetNeighbor)};
  for (ULONG i = 0; i < table->NumEntries; i++) {
    const MIB_IPNET_ROW2* row = &table->Table[i];
    uint8_t dst[16];
    int f;
    if (!sockaddr_inet_bytes(&row->Address, &f, dst))
      continue;
    /* Linux keeps no entries for multicast and broadcast groups */
    if (f == NQ_NET_INET ? (dst[0] & 0xF0) >= 0xE0 : dst[0] == 0xFF)
      continue;
    NQNetNeighbor* n = (NQNetNeighbor*)list_add(&list);
    if (n == NULL) {
      FreeMibTable(table);
      NQFree(list.items);
      return -NQ_ENOMEM;
    }
    n->family = f;
    NQMemcpy(n->dst, dst, 16);
    n->index = row->InterfaceIndex;
    n->state = linux_neigh_state(row->State);
    n->have = NQ_NET_NEIGH_DST;
    if (row->PhysicalAddressLength > 0 && row->PhysicalAddressLength <= NQ_NET_HWADDR_SIZE &&
        n->state != NQ_NET_NUD_INCOMPLETE && n->state != NQ_NET_NUD_FAILED) {
      n->lladdrLength = row->PhysicalAddressLength;
      NQMemcpy(n->lladdr, row->PhysicalAddress, row->PhysicalAddressLength);
      n->have |= NQ_NET_NEIGH_LLADDR;
    }
    if (row->IsRouter)
      n->flags |= NQ_NET_NTF_ROUTER;
  }
  FreeMibTable(table);
  *neighbors = (NQNetNeighbor*)list.items;
  *count = list.count;
  return 0;
}

#endif /* NQ_OS_WINDOWS */
