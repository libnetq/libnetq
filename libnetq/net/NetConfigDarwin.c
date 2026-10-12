/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

/* NetConfig.h on macOS: getifaddrs for interfaces and addresses, the
 * routing sysctls for routes and the neighbour cache. Read only. iOS has
 * the interfaces and addresses only: its SDK lacks net/route.h, and the
 * system keeps the routing tables and the neighbour cache from apps. */

#include "config.h"

#include "libnetq/net/NetConfig.h"

#if defined(NQ_OS_DARWIN)

#include <libnetq/ErrorCode.h>
#include <libnetq/Malloc.h>
#include <libnetq/string/String.h>

#include <errno.h>
#include <stddef.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#if defined(NQ_OS_MACOSX)
# include <net/route.h>
# include <sys/sysctl.h>
# include <time.h>
#endif

/* Copies a NUL-terminated name, cut to fit */
static void copy_name(char* out, const char* name, size_t size)
{
  size_t n = NQStrlen(name);
  if (n >= size)
    n = size - 1;
  NQMemcpy(out, name, n);
  out[n] = '\0';
}

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

/* BSD's interface flags as Linux's */
static unsigned linux_flags(unsigned bsd)
{
  static const struct {
    unsigned bsd, nq;
  } map[] = {
    {IFF_UP, NQ_NET_IFF_UP},
    {IFF_BROADCAST, NQ_NET_IFF_BROADCAST},
    {IFF_DEBUG, NQ_NET_IFF_DEBUG},
    {IFF_LOOPBACK, NQ_NET_IFF_LOOPBACK},
    {IFF_POINTOPOINT, NQ_NET_IFF_POINTOPOINT},
    {IFF_RUNNING, NQ_NET_IFF_RUNNING},
    {IFF_NOARP, NQ_NET_IFF_NOARP},
    {IFF_PROMISC, NQ_NET_IFF_PROMISC},
    {IFF_ALLMULTI, NQ_NET_IFF_ALLMULTI},
    {IFF_MULTICAST, NQ_NET_IFF_MULTICAST},
  };
  unsigned flags = 0;
  for (size_t i = 0; i < NQ_ARRAY_LENGTH(map); i++) {
    if (bsd & map[i].bsd)
      flags |= map[i].nq;
  }
  if (flags & NQ_NET_IFF_RUNNING)
    flags |= NQ_NET_IFF_LOWER_UP;
  return flags;
}

static unsigned linux_type(unsigned char type)
{
  switch (type) {
  case IFT_ETHER:
  case IFT_BRIDGE:
  case IFT_L2VLAN:
  case IFT_IEEE8023ADLAG:
    return NQ_NET_ARPHRD_ETHER;
  case IFT_LOOP:
    return NQ_NET_ARPHRD_LOOPBACK;
  case IFT_PPP:
    return NQ_NET_ARPHRD_PPP;
  case IFT_GIF:
    return NQ_NET_ARPHRD_TUNNEL6;
  default:
    return NQ_NET_ARPHRD_NONE;
  }
}

int NQNetGetLinks(NQNetLink** links, size_t* count)
{
  *links = NULL;
  *count = 0;
  struct ifaddrs* all = NULL;
  if (getifaddrs(&all) != 0)
    return -errno;

  List list = {NULL, 0, 0, sizeof(NQNetLink)};
  for (struct ifaddrs* i = all; i != NULL; i = i->ifa_next) {
    if (i->ifa_addr == NULL || i->ifa_addr->sa_family != AF_LINK)
      continue;
    const struct sockaddr_dl* sdl = (const struct sockaddr_dl*)i->ifa_addr;
    NQNetLink* l = (NQNetLink*)list_add(&list);
    if (l == NULL) {
      freeifaddrs(all);
      NQFree(list.items);
      return -NQ_ENOMEM;
    }
    l->index = sdl->sdl_index;
    copy_name(l->name, i->ifa_name, sizeof(l->name));
    l->flags = linux_flags(i->ifa_flags);
    l->type = linux_type(sdl->sdl_type);
    l->operState = (i->ifa_flags & IFF_RUNNING) ? NQ_NET_OPER_UP : NQ_NET_OPER_DOWN;
    l->have |= NQ_NET_LINK_OPERSTATE;
    if (sdl->sdl_alen > 0 && sdl->sdl_alen <= NQ_NET_HWADDR_SIZE) {
      l->addressLength = sdl->sdl_alen;
      NQMemcpy(l->address, LLADDR(sdl), sdl->sdl_alen);
      if (l->flags & NQ_NET_IFF_BROADCAST) {
        l->broadcastLength = sdl->sdl_alen;
        NQMemset(l->broadcast, 0xFF, sdl->sdl_alen);
      }
    } else if (l->type == NQ_NET_ARPHRD_LOOPBACK) {
      l->addressLength = l->broadcastLength = 6;
    }
    const struct if_data* data = (const struct if_data*)i->ifa_data;
    if (data != NULL) {
      l->mtu = data->ifi_mtu;
      l->have |= NQ_NET_LINK_MTU | NQ_NET_LINK_STATS;
      l->stats.rxPackets = data->ifi_ipackets;
      l->stats.txPackets = data->ifi_opackets;
      l->stats.rxBytes = data->ifi_ibytes;
      l->stats.txBytes = data->ifi_obytes;
      l->stats.rxErrors = data->ifi_ierrors;
      l->stats.txErrors = data->ifi_oerrors;
      l->stats.rxDropped = data->ifi_iqdrops;
      l->stats.multicast = data->ifi_imcasts;
      l->stats.collisions = data->ifi_collisions;
    }
  }
  freeifaddrs(all);
  *links = (NQNetLink*)list.items;
  *count = list.count;
  return 0;
}

static unsigned prefix_length(const uint8_t* mask, size_t bytes)
{
  unsigned bits = 0;
  for (size_t i = 0; i < bytes; i++) {
    uint8_t b = mask[i];
    while (b & 0x80) {
      bits++;
      b = (uint8_t)(b << 1);
    }
    if (mask[i] != 0xFF)
      break;
  }
  return bits;
}

/* KAME keeps the scope in bytes 2 and 3 of link-local addresses */
static void clear_embedded_scope(uint8_t* a)
{
  if (a[0] == 0xfe && (a[1] & 0xc0) == 0x80)
    a[2] = a[3] = 0;
}

int NQNetGetAddresses(int family, NQNetAddress** addresses, size_t* count)
{
  *addresses = NULL;
  *count = 0;
  struct ifaddrs* all = NULL;
  if (getifaddrs(&all) != 0)
    return -errno;

  List list = {NULL, 0, 0, sizeof(NQNetAddress)};
  for (struct ifaddrs* i = all; i != NULL; i = i->ifa_next) {
    if (i->ifa_addr == NULL)
      continue;
    int f = i->ifa_addr->sa_family == AF_INET ? NQ_NET_INET : i->ifa_addr->sa_family == AF_INET6 ? NQ_NET_INET6 : 0;
    if (f == 0 || (family && family != f))
      continue;
    NQNetAddress* a = (NQNetAddress*)list_add(&list);
    if (a == NULL) {
      freeifaddrs(all);
      NQFree(list.items);
      return -NQ_ENOMEM;
    }
    a->index = if_nametoindex(i->ifa_name);
    a->family = f;
    a->flags = NQ_NET_IFA_PERMANENT;
    a->have = NQ_NET_ADDR_LOCAL | NQ_NET_ADDR_ADDRESS;
    if (f == NQ_NET_INET) {
      NQMemcpy(a->local, &((const struct sockaddr_in*)i->ifa_addr)->sin_addr, 4);
      NQMemcpy(a->address, a->local, 4);
      if (i->ifa_netmask)
        a->prefixLength = (uint8_t)prefix_length((const uint8_t*)&((const struct sockaddr_in*)i->ifa_netmask)->sin_addr, 4);
      if ((i->ifa_flags & IFF_POINTOPOINT) && i->ifa_dstaddr)
        NQMemcpy(a->address, &((const struct sockaddr_in*)i->ifa_dstaddr)->sin_addr, 4);
      else if ((i->ifa_flags & IFF_BROADCAST) && i->ifa_broadaddr) {
        NQMemcpy(a->broadcast, &((const struct sockaddr_in*)i->ifa_broadaddr)->sin_addr, 4);
        a->have |= NQ_NET_ADDR_BROADCAST;
      }
      a->scope = a->local[0] == 127 ? NQ_NET_SCOPE_HOST : NQ_NET_SCOPE_UNIVERSE;
      copy_name(a->label, i->ifa_name, sizeof(a->label));
      a->have |= NQ_NET_ADDR_LABEL;
    } else {
      NQMemcpy(a->local, &((const struct sockaddr_in6*)i->ifa_addr)->sin6_addr, 16);
      clear_embedded_scope(a->local);
      NQMemcpy(a->address, a->local, 16);
      if (i->ifa_netmask)
        a->prefixLength = (uint8_t)prefix_length((const uint8_t*)&((const struct sockaddr_in6*)i->ifa_netmask)->sin6_addr, 16);
      static const uint8_t loopback[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
      if (NQMemcmp(a->local, loopback, 16) == 0)
        a->scope = NQ_NET_SCOPE_HOST;
      else if (a->local[0] == 0xfe && (a->local[1] & 0xc0) == 0x80)
        a->scope = NQ_NET_SCOPE_LINK;
      else
        a->scope = NQ_NET_SCOPE_UNIVERSE;
    }
  }
  freeifaddrs(all);
  *addresses = (NQNetAddress*)list.items;
  *count = list.count;
  return 0;
}

#if !defined(NQ_OS_MACOSX)

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

#else

/* Reads a routing sysctl into a buffer allocated with NQMalloc */
static int route_sysctl(int family, int what, int arg, char** buffer, size_t* length)
{
  int mib[6] = {CTL_NET, PF_ROUTE, 0, family, what, arg};
  for (int attempt = 0; attempt < 5; attempt++) {
    size_t size = 0;
    if (sysctl(mib, 6, NULL, &size, NULL, 0) < 0)
      return -errno;
    size += size / 4 + 1024;
    char* buf = (char*)NQMalloc(size);
    if (buf == NULL)
      return -NQ_ENOMEM;
    if (sysctl(mib, 6, buf, &size, NULL, 0) == 0) {
      *buffer = buf;
      *length = size;
      return 0;
    }
    int err = errno;
    NQFree(buf);
    if (err != ENOMEM)
      return -err;
  }
  return -NQ_ENOMEM;
}

#define SA_SIZE(sa) ((sa)->sa_len ? (((sa)->sa_len + sizeof(uint32_t) - 1) & ~(sizeof(uint32_t) - 1)) : sizeof(uint32_t))

/* The socket addresses after a routing message header, by RTA_* bit */
static void split_addrs(const char* p, const char* end, int addrs, const struct sockaddr* sa[RTAX_MAX])
{
  for (int i = 0; i < RTAX_MAX; i++) {
    sa[i] = NULL;
    if (!(addrs & (1 << i)) || p >= end)
      continue;
    sa[i] = (const struct sockaddr*)p;
    p += SA_SIZE(sa[i]);
  }
}

/* An address from a sockaddr, or false for another family */
static bool sockaddr_bytes(const struct sockaddr* sa, int family, uint8_t* out)
{
  if (sa == NULL)
    return false;
  if (family == NQ_NET_INET && sa->sa_family == AF_INET) {
    NQMemcpy(out, &((const struct sockaddr_in*)sa)->sin_addr, 4);
    return true;
  }
  if (family == NQ_NET_INET6 && sa->sa_family == AF_INET6) {
    NQMemcpy(out, &((const struct sockaddr_in6*)sa)->sin6_addr, 16);
    clear_embedded_scope(out);
    return true;
  }
  return false;
}

/* A netmask may come cut short after its last non-zero byte */
static unsigned mask_length(const struct sockaddr* sa, int family)
{
  if (sa == NULL)
    return family == NQ_NET_INET ? 32 : 128;
  uint8_t mask[16] = {0};
  size_t offset = family == NQ_NET_INET ? offsetof(struct sockaddr_in, sin_addr) : offsetof(struct sockaddr_in6, sin6_addr);
  size_t bytes = family == NQ_NET_INET ? 4 : 16;
  if (sa->sa_len > offset) {
    size_t n = sa->sa_len - offset;
    NQMemcpy(mask, (const uint8_t*)sa + offset, n < bytes ? n : bytes);
  }
  return prefix_length(mask, bytes);
}

static int get_routes(int family, List* list)
{
  char* buf;
  size_t len;
  int ret = route_sysctl(family == NQ_NET_INET ? AF_INET : AF_INET6, NET_RT_DUMP, 0, &buf, &len);
  if (ret != 0)
    return ret;
  size_t bytes = family == NQ_NET_INET ? 4 : 16;
  for (char* p = buf; p + sizeof(struct rt_msghdr) <= buf + len;) {
    const struct rt_msghdr* rtm = (const struct rt_msghdr*)p;
    if (rtm->rtm_msglen == 0)
      break;
    char* next = p + rtm->rtm_msglen;
    p = next;
    /* the neighbour cache and the host routes it clones */
    if (!(rtm->rtm_flags & RTF_UP) || (rtm->rtm_flags & (RTF_LLINFO | RTF_WASCLONED)))
      continue;
    const struct sockaddr* sa[RTAX_MAX];
    split_addrs((const char*)(rtm + 1), next, rtm->rtm_addrs, sa);

    NQNetRoute* r = (NQNetRoute*)list_add(list);
    if (r == NULL) {
      NQFree(buf);
      return -NQ_ENOMEM;
    }
    r->family = family;
    r->table = NQ_NET_TABLE_MAIN;
    r->type = NQ_NET_RTN_UNICAST;
    r->protocol = (rtm->rtm_flags & RTF_STATIC) ? NQ_NET_RTPROT_STATIC : NQ_NET_RTPROT_KERNEL;
    if (sockaddr_bytes(sa[RTAX_DST], family, r->dst)) {
      r->dstLength = (rtm->rtm_flags & RTF_HOST) ? (uint8_t)(bytes * 8) : (uint8_t)mask_length(sa[RTAX_NETMASK], family);
      if (r->dstLength)
        r->have |= NQ_NET_ROUTE_DST;
    }
    r->oif = rtm->rtm_index;
    r->have |= NQ_NET_ROUTE_OIF;
    if ((rtm->rtm_flags & RTF_GATEWAY) && sockaddr_bytes(sa[RTAX_GATEWAY], family, r->gateway)) {
      r->have |= NQ_NET_ROUTE_GATEWAY;
      r->scope = NQ_NET_SCOPE_UNIVERSE;
    } else {
      r->scope = family == NQ_NET_INET ? NQ_NET_SCOPE_LINK : NQ_NET_SCOPE_UNIVERSE;
    }
    if (rtm->rtm_flags & RTF_LOCAL) {
      r->type = NQ_NET_RTN_LOCAL;
      r->table = NQ_NET_TABLE_LOCAL;
      r->scope = NQ_NET_SCOPE_HOST;
    } else if (rtm->rtm_flags & RTF_BROADCAST) {
      r->type = NQ_NET_RTN_BROADCAST;
      r->table = NQ_NET_TABLE_LOCAL;
    } else if (rtm->rtm_flags & RTF_MULTICAST) {
      r->type = NQ_NET_RTN_MULTICAST;
      r->table = NQ_NET_TABLE_LOCAL;
    } else if (rtm->rtm_flags & RTF_REJECT) {
      r->type = (rtm->rtm_flags & RTF_BLACKHOLE) ? NQ_NET_RTN_BLACKHOLE : NQ_NET_RTN_UNREACHABLE;
    }
    if (rtm->rtm_rmx.rmx_mtu) {
      r->metrics[NQ_NET_RTAX_MTU] = rtm->rtm_rmx.rmx_mtu;
      r->metricsHave |= 1u << NQ_NET_RTAX_MTU;
      r->have |= NQ_NET_ROUTE_METRICS;
    }
  }
  NQFree(buf);
  return 0;
}

int NQNetGetRoutes(int family, NQNetRoute** routes, size_t* count)
{
  *routes = NULL;
  *count = 0;
  List list = {NULL, 0, 0, sizeof(NQNetRoute)};
  int ret = 0;
  if (family != NQ_NET_INET6)
    ret = get_routes(NQ_NET_INET, &list);
  if (ret == 0 && family != NQ_NET_INET)
    ret = get_routes(NQ_NET_INET6, &list);
  if (ret != 0) {
    NQNetFreeRoutes((NQNetRoute*)list.items, list.count);
    return ret;
  }
  *routes = (NQNetRoute*)list.items;
  *count = list.count;
  return 0;
}

static int get_neighbors(int family, List* list)
{
  char* buf;
  size_t len;
  int ret = route_sysctl(family == NQ_NET_INET ? AF_INET : AF_INET6, NET_RT_FLAGS, RTF_LLINFO, &buf, &len);
  if (ret != 0)
    return ret;
  time_t now = time(NULL);
  for (char* p = buf; p + sizeof(struct rt_msghdr) <= buf + len;) {
    const struct rt_msghdr* rtm = (const struct rt_msghdr*)p;
    if (rtm->rtm_msglen == 0)
      break;
    char* next = p + rtm->rtm_msglen;
    p = next;
    const struct sockaddr* sa[RTAX_MAX];
    split_addrs((const char*)(rtm + 1), next, rtm->rtm_addrs, sa);
    const struct sockaddr_dl* sdl = (const struct sockaddr_dl*)sa[RTAX_GATEWAY];
    if (sdl == NULL || sdl->sdl_family != AF_LINK)
      continue;

    NQNetNeighbor* n = (NQNetNeighbor*)list_add(list);
    if (n == NULL) {
      NQFree(buf);
      return -NQ_ENOMEM;
    }
    n->family = family;
    if (sockaddr_bytes(sa[RTAX_DST], family, n->dst))
      n->have |= NQ_NET_NEIGH_DST;
    n->index = sdl->sdl_index;
    if (sdl->sdl_alen > 0 && sdl->sdl_alen <= NQ_NET_HWADDR_SIZE) {
      n->lladdrLength = sdl->sdl_alen;
      NQMemcpy(n->lladdr, LLADDR(sdl), sdl->sdl_alen);
      n->have |= NQ_NET_NEIGH_LLADDR;
      if (rtm->rtm_rmx.rmx_expire == 0)
        n->state = NQ_NET_NUD_PERMANENT;
      else
        n->state = rtm->rtm_rmx.rmx_expire > now ? NQ_NET_NUD_REACHABLE : NQ_NET_NUD_STALE;
    } else {
      n->state = NQ_NET_NUD_INCOMPLETE;
    }
    if (rtm->rtm_flags & RTF_ROUTER)
      n->flags |= NQ_NET_NTF_ROUTER;
  }
  NQFree(buf);
  return 0;
}

int NQNetGetNeighbors(int family, NQNetNeighbor** neighbors, size_t* count)
{
  *neighbors = NULL;
  *count = 0;
  List list = {NULL, 0, 0, sizeof(NQNetNeighbor)};
  int ret = 0;
  if (family != NQ_NET_INET6)
    ret = get_neighbors(NQ_NET_INET, &list);
  if (ret == 0 && family != NQ_NET_INET)
    ret = get_neighbors(NQ_NET_INET6, &list);
  if (ret != 0) {
    NQFree(list.items);
    return ret;
  }
  *neighbors = (NQNetNeighbor*)list.items;
  *count = list.count;
  return 0;
}

#endif /* NQ_OS_MACOSX */

#endif /* NQ_OS_DARWIN */
