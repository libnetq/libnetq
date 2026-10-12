/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

/* NetConfig.h on Linux and Android: rtnetlink, as iproute2 talks it.
 * Attribute numbers are spelled out here, since older kernel headers
 * (the NDK's) lack the newer ones. */

#include "config.h"

#include "libnetq/net/NetConfig.h"

#if defined(NQ_OS_LINUX)

#include <libnetq/ErrorCode.h>
#include <libnetq/Malloc.h>
#include <libnetq/string/String.h>

#include <errno.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <unistd.h>

#define NL_BUFFER_SIZE 65536
#define NL_REQUEST_SIZE 4096

/* netlink options the NDK's headers may not have */
#define NL_EXT_ACK 11
#define NL_F_CAPPED 0x100
#define NL_F_ACK_TLVS 0x200
#define NL_ERR_ATTR_MSG 1

enum {
  A_IFLA_ADDRESS = 1,
  A_IFLA_BROADCAST = 2,
  A_IFLA_IFNAME = 3,
  A_IFLA_MTU = 4,
  A_IFLA_LINK = 5,
  A_IFLA_QDISC = 6,
  A_IFLA_STATS = 7,
  A_IFLA_MASTER = 10,
  A_IFLA_TXQLEN = 13,
  A_IFLA_OPERSTATE = 16,
  A_IFLA_LINKMODE = 17,
  A_IFLA_LINKINFO = 18,
  A_IFLA_IFALIAS = 20,
  A_IFLA_STATS64 = 23,
  A_IFLA_GROUP = 27,
  A_IFLA_PROMISCUITY = 30,
  A_IFLA_CARRIER = 33,
  A_IFLA_CARRIER_CHANGES = 35,
  A_IFLA_LINK_NETNSID = 37,
  A_IFLA_PROTO_DOWN = 39,
  A_IFLA_MIN_MTU = 50,
  A_IFLA_MAX_MTU = 51,
  A_IFLA_PERM_ADDRESS = 54,
  A_IFLA_ALLMULTI = 61,
  A_IFLA_INFO_KIND = 1,
  A_IFLA_INFO_DATA = 2,
  A_VETH_INFO_PEER = 1,

  A_IFA_ADDRESS = 1,
  A_IFA_LOCAL = 2,
  A_IFA_LABEL = 3,
  A_IFA_BROADCAST = 4,
  A_IFA_ANYCAST = 5,
  A_IFA_CACHEINFO = 6,
  A_IFA_FLAGS = 8,
  A_IFA_RT_PRIORITY = 9,

  A_RTA_DST = 1,
  A_RTA_SRC = 2,
  A_RTA_IIF = 3,
  A_RTA_OIF = 4,
  A_RTA_GATEWAY = 5,
  A_RTA_PRIORITY = 6,
  A_RTA_PREFSRC = 7,
  A_RTA_METRICS = 8,
  A_RTA_MULTIPATH = 9,
  A_RTA_FLOW = 11,
  A_RTA_CACHEINFO = 12,
  A_RTA_TABLE = 15,
  A_RTA_MARK = 16,
  A_RTA_VIA = 18,
  A_RTA_PREF = 20,
  A_RTA_EXPIRES = 23,
  A_RTA_UID = 25,
  A_RTA_NH_ID = 30,

  A_NDA_DST = 1,
  A_NDA_LLADDR = 2,
  A_NDA_CACHEINFO = 3,
  A_NDA_PROBES = 4,
  A_NDA_PROTOCOL = 12,
  A_NDA_FLAGS_EXT = 15,
};

/* RTM_F_LOOKUP_TABLE and RTM_F_FIB_MATCH */
#define A_RTM_F_LOOKUP_TABLE 0x1000
#define A_RTM_F_FIB_MATCH 0x2000

/* The neighbour message, from linux/neighbour.h */
struct nq_ndmsg {
  uint8_t ndm_family;
  uint8_t ndm_pad1;
  uint16_t ndm_pad2;
  int32_t ndm_ifindex;
  uint16_t ndm_state;
  uint8_t ndm_flags;
  uint8_t ndm_type;
};

static __thread char error_message[256];

const char* NQNetGetErrorMessage(void)
{
  return error_message;
}

/* --- the socket ----------------------------------------------------------- */

typedef struct {
  int fd;
  uint32_t seq;
  char* buffer;
} Netlink;

static int nl_open(Netlink* nl)
{
  error_message[0] = '\0';
  nl->buffer = (char*)NQMalloc(NL_BUFFER_SIZE);
  if (nl->buffer == NULL)
    return -NQ_ENOMEM;
  nl->fd = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);
  if (nl->fd < 0) {
    int err = errno;
    NQFree(nl->buffer);
    return -err;
  }
  int one = 1;
  setsockopt(nl->fd, SOL_NETLINK, NL_EXT_ACK, &one, sizeof(one));
  struct sockaddr_nl local;
  NQMemset(&local, 0, sizeof(local));
  local.nl_family = AF_NETLINK;
  if (bind(nl->fd, (struct sockaddr*)&local, sizeof(local)) < 0) {
    int err = errno;
    close(nl->fd);
    NQFree(nl->buffer);
    return -err;
  }
  nl->seq = 1;
  return 0;
}

static void nl_close(Netlink* nl)
{
  close(nl->fd);
  NQFree(nl->buffer);
}

static int nl_send(Netlink* nl, struct nlmsghdr* n)
{
  struct sockaddr_nl kernel;
  NQMemset(&kernel, 0, sizeof(kernel));
  kernel.nl_family = AF_NETLINK;
  n->nlmsg_seq = ++nl->seq;
  ssize_t sent;
  do {
    sent = sendto(nl->fd, n, n->nlmsg_len, 0, (struct sockaddr*)&kernel, sizeof(kernel));
  } while (sent < 0 && errno == EINTR);
  return sent < 0 ? -errno : 0;
}

static ssize_t nl_receive(Netlink* nl)
{
  ssize_t got;
  do {
    got = recv(nl->fd, nl->buffer, NL_BUFFER_SIZE, 0);
  } while (got < 0 && errno == EINTR);
  return got < 0 ? -errno : got;
}

/* Keeps the kernel's own words about an error */
static void keep_error_message(const struct nlmsghdr* h)
{
  const struct nlmsgerr* err = (const struct nlmsgerr*)NLMSG_DATA(h);
  if (!(h->nlmsg_flags & NL_F_ACK_TLVS))
    return;
  size_t offset = NLMSG_HDRLEN + sizeof(*err);
  if (!(h->nlmsg_flags & NL_F_CAPPED))
    offset += err->msg.nlmsg_len - NLMSG_HDRLEN;
  offset = NLMSG_ALIGN(offset);
  while (offset + sizeof(struct nlattr) <= h->nlmsg_len) {
    const struct nlattr* a = (const struct nlattr*)((const char*)h + offset);
    if (a->nla_len < sizeof(*a) || offset + a->nla_len > h->nlmsg_len)
      break;
    if ((a->nla_type & NLA_TYPE_MASK) == NL_ERR_ATTR_MSG) {
      size_t len = a->nla_len - sizeof(*a);
      if (len >= sizeof(error_message))
        len = sizeof(error_message) - 1;
      NQMemcpy(error_message, (const char*)a + sizeof(*a), len);
      error_message[len] = '\0';
      return;
    }
    offset += NLA_ALIGN(a->nla_len);
  }
}

typedef int (*MessageFn)(struct nlmsghdr* h, void* ctx);

/* Sends a dump request and hands each answer to `fn` */
static int nl_dump(Netlink* nl, struct nlmsghdr* request, MessageFn fn, void* ctx)
{
  request->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
  int ret = nl_send(nl, request);
  if (ret != 0)
    return ret;
  for (;;) {
    ssize_t got = nl_receive(nl);
    if (got < 0)
      return (int)got;
    if (got == 0)
      return -NQ_EIO;
    for (struct nlmsghdr* h = (struct nlmsghdr*)nl->buffer; NLMSG_OK(h, (unsigned)got); h = NLMSG_NEXT(h, got)) {
      if (h->nlmsg_seq != nl->seq)
        continue;
      if (h->nlmsg_type == NLMSG_DONE)
        return 0;
      if (h->nlmsg_type == NLMSG_ERROR) {
        const struct nlmsgerr* err = (const struct nlmsgerr*)NLMSG_DATA(h);
        keep_error_message(h);
        return err->error;
      }
      ret = fn(h, ctx);
      if (ret != 0)
        return ret;
    }
  }
}

/* Sends a request and waits for its acknowledgement; an answer before it
 * goes to `fn` */
static int nl_talk(Netlink* nl, struct nlmsghdr* request, MessageFn fn, void* ctx)
{
  request->nlmsg_flags |= NLM_F_REQUEST | NLM_F_ACK;
  int ret = nl_send(nl, request);
  if (ret != 0)
    return ret;
  for (;;) {
    ssize_t got = nl_receive(nl);
    if (got < 0)
      return (int)got;
    if (got == 0)
      return -NQ_EIO;
    for (struct nlmsghdr* h = (struct nlmsghdr*)nl->buffer; NLMSG_OK(h, (unsigned)got); h = NLMSG_NEXT(h, got)) {
      if (h->nlmsg_seq != nl->seq)
        continue;
      if (h->nlmsg_type == NLMSG_ERROR) {
        const struct nlmsgerr* err = (const struct nlmsgerr*)NLMSG_DATA(h);
        if (err->error != 0)
          keep_error_message(h);
        return err->error;
      }
      if (h->nlmsg_type == NLMSG_DONE)
        return 0;
      if (fn != NULL) {
        ret = fn(h, ctx);
        if (ret != 0)
          return ret;
      }
    }
  }
}

/* --- building requests ---------------------------------------------------- */

typedef struct {
  struct nlmsghdr n;
  char data[NL_REQUEST_SIZE];
} Request;

static void* request_init(Request* r, int type, size_t header)
{
  NQMemset(r, 0, sizeof(*r));
  r->n.nlmsg_len = NLMSG_LENGTH(header);
  r->n.nlmsg_type = (uint16_t)type;
  return NLMSG_DATA(&r->n);
}

static struct rtattr* add_attr(struct nlmsghdr* n, int type, const void* data, size_t len)
{
  size_t total = NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(RTA_LENGTH(len));
  if (total > sizeof(Request))
    return NULL;
  struct rtattr* a = (struct rtattr*)((char*)n + NLMSG_ALIGN(n->nlmsg_len));
  a->rta_type = (unsigned short)type;
  a->rta_len = (unsigned short)RTA_LENGTH(len);
  if (len && data)
    NQMemcpy(RTA_DATA(a), data, len);
  else if (len)
    NQMemset(RTA_DATA(a), 0, len);
  n->nlmsg_len = (uint32_t)total;
  return a;
}

static void add_u8(struct nlmsghdr* n, int type, uint8_t v)
{
  add_attr(n, type, &v, 1);
}

static void add_u32(struct nlmsghdr* n, int type, uint32_t v)
{
  add_attr(n, type, &v, 4);
}

static void add_string(struct nlmsghdr* n, int type, const char* s, bool nul)
{
  add_attr(n, type, s, NQStrlen(s) + (nul ? 1 : 0));
}

static struct rtattr* nest_begin(struct nlmsghdr* n, int type)
{
  return add_attr(n, type, NULL, 0);
}

static void nest_end(struct nlmsghdr* n, struct rtattr* nest)
{
  if (nest)
    nest->rta_len = (unsigned short)((char*)n + NLMSG_ALIGN(n->nlmsg_len) - (char*)nest);
}

static int nl_family(int family)
{
  return family == NQ_NET_INET ? AF_INET : family == NQ_NET_INET6 ? AF_INET6 : AF_UNSPEC;
}

static int nq_family(int family)
{
  return family == AF_INET ? NQ_NET_INET : family == AF_INET6 ? NQ_NET_INET6 : NQ_NET_UNSPEC;
}

static size_t family_bytes(int family)
{
  return family == NQ_NET_INET6 ? 16 : 4;
}

static uint16_t change_flags(NQNetChange change)
{
  switch (change) {
  case NQ_NET_ADD: return NLM_F_CREATE | NLM_F_EXCL;
  case NQ_NET_CHANGE: return NLM_F_REPLACE;
  case NQ_NET_REPLACE: return NLM_F_CREATE | NLM_F_REPLACE;
  case NQ_NET_PREPEND: return NLM_F_CREATE;
  case NQ_NET_APPEND: return NLM_F_CREATE | NLM_F_APPEND;
  case NQ_NET_TEST: return NLM_F_EXCL;
  default: return 0;
  }
}

/* Runs one change request: opens the socket, talks and closes it */
static int run_request(Request* r)
{
  Netlink nl;
  int ret = nl_open(&nl);
  if (ret != 0)
    return ret;
  ret = nl_talk(&nl, &r->n, NULL, NULL);
  nl_close(&nl);
  return ret;
}

/* --- parsing answers ------------------------------------------------------ */

#define MAX_ATTR 64

static void parse_attrs(struct rtattr* tb[MAX_ATTR], struct rtattr* a, int len)
{
  NQMemset(tb, 0, sizeof(struct rtattr*) * MAX_ATTR);
  for (; RTA_OK(a, len); a = RTA_NEXT(a, len)) {
    unsigned type = a->rta_type & NLA_TYPE_MASK;
    if (type < MAX_ATTR && tb[type] == NULL)
      tb[type] = a;
  }
}

static uint32_t attr_u32(const struct rtattr* a)
{
  uint32_t v = 0;
  size_t len = RTA_PAYLOAD(a);
  NQMemcpy(&v, RTA_DATA(a), len < 4 ? len : 4);
  return v;
}

static uint8_t attr_u8(const struct rtattr* a)
{
  return RTA_PAYLOAD(a) ? *(const uint8_t*)RTA_DATA(a) : 0;
}

static void attr_string(const struct rtattr* a, char* out, size_t size)
{
  size_t len = RTA_PAYLOAD(a);
  const char* s = (const char*)RTA_DATA(a);
  size_t n = 0;
  while (n < len && n + 1 < size && s[n])
    n++;
  NQMemcpy(out, s, n);
  out[n] = '\0';
}

static size_t attr_bytes(const struct rtattr* a, uint8_t* out, size_t size)
{
  size_t len = RTA_PAYLOAD(a);
  if (len > size)
    len = size;
  NQMemcpy(out, RTA_DATA(a), len);
  return len;
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

static long user_hz(void)
{
  long hz = sysconf(_SC_CLK_TCK);
  return hz > 0 ? hz : 100;
}

/* --- links ---------------------------------------------------------------- */

/* The counters, in the order of struct rtnl_link_stats64 */
static void parse_stats(NQNetLinkStats* s, const struct rtattr* a, bool wide)
{
  uint64_t* fields[] = {
    &s->rxPackets, &s->txPackets, &s->rxBytes, &s->txBytes, &s->rxErrors, &s->txErrors,
    &s->rxDropped, &s->txDropped, &s->multicast, &s->collisions, &s->rxLengthErrors,
    &s->rxOverErrors, &s->rxCrcErrors, &s->rxFrameErrors, &s->rxFifoErrors, &s->rxMissedErrors,
    &s->txAbortedErrors, &s->txCarrierErrors, &s->txFifoErrors, &s->txHeartbeatErrors,
    &s->txWindowErrors, &s->rxCompressed, &s->txCompressed, &s->rxNohandler,
    &s->rxOtherhostDropped,
  };
  size_t width = wide ? 8 : 4;
  size_t n = RTA_PAYLOAD(a) / width;
  const uint8_t* p = (const uint8_t*)RTA_DATA(a);
  for (size_t i = 0; i < n && i < NQ_ARRAY_LENGTH(fields); i++) {
    if (wide) {
      uint64_t v;
      NQMemcpy(&v, p + i * 8, 8);
      *fields[i] = v;
    } else {
      uint32_t v;
      NQMemcpy(&v, p + i * 4, 4);
      *fields[i] = v;
    }
  }
}

static int parse_link(struct nlmsghdr* h, void* ctx)
{
  if (h->nlmsg_type != RTM_NEWLINK)
    return 0;
  struct ifinfomsg* ifi = (struct ifinfomsg*)NLMSG_DATA(h);
  int len = (int)h->nlmsg_len - NLMSG_LENGTH(sizeof(*ifi));
  if (len < 0)
    return 0;
  struct rtattr* tb[MAX_ATTR];
  parse_attrs(tb, IFLA_RTA(ifi), len);

  NQNetLink* l = (NQNetLink*)list_add((List*)ctx);
  if (l == NULL)
    return -NQ_ENOMEM;
  l->index = (unsigned)ifi->ifi_index;
  l->flags = ifi->ifi_flags;
  l->type = ifi->ifi_type;
  if (tb[A_IFLA_IFNAME])
    attr_string(tb[A_IFLA_IFNAME], l->name, sizeof(l->name));
  if (tb[A_IFLA_MTU]) {
    l->mtu = attr_u32(tb[A_IFLA_MTU]);
    l->have |= NQ_NET_LINK_MTU;
  }
  if (tb[A_IFLA_QDISC]) {
    attr_string(tb[A_IFLA_QDISC], l->qdisc, sizeof(l->qdisc));
    l->have |= NQ_NET_LINK_QDISC;
  }
  if (tb[A_IFLA_MASTER]) {
    l->master = attr_u32(tb[A_IFLA_MASTER]);
    l->have |= NQ_NET_LINK_MASTER;
  }
  if (tb[A_IFLA_LINK]) {
    l->link = attr_u32(tb[A_IFLA_LINK]);
    l->have |= NQ_NET_LINK_LINK;
  }
  if (tb[A_IFLA_LINK_NETNSID]) {
    l->linkNetnsid = (int)attr_u32(tb[A_IFLA_LINK_NETNSID]);
    l->have |= NQ_NET_LINK_NETNSID;
  }
  if (tb[A_IFLA_OPERSTATE]) {
    l->operState = attr_u8(tb[A_IFLA_OPERSTATE]);
    l->have |= NQ_NET_LINK_OPERSTATE;
  }
  if (tb[A_IFLA_LINKMODE]) {
    l->linkMode = attr_u8(tb[A_IFLA_LINKMODE]);
    l->have |= NQ_NET_LINK_LINKMODE;
  }
  if (tb[A_IFLA_GROUP]) {
    l->group = attr_u32(tb[A_IFLA_GROUP]);
    l->have |= NQ_NET_LINK_GROUP;
  }
  if (tb[A_IFLA_TXQLEN]) {
    l->txqlen = attr_u32(tb[A_IFLA_TXQLEN]);
    l->have |= NQ_NET_LINK_TXQLEN;
  }
  if (tb[A_IFLA_ADDRESS])
    l->addressLength = attr_bytes(tb[A_IFLA_ADDRESS], l->address, sizeof(l->address));
  if (tb[A_IFLA_BROADCAST])
    l->broadcastLength = attr_bytes(tb[A_IFLA_BROADCAST], l->broadcast, sizeof(l->broadcast));
  if (tb[A_IFLA_PERM_ADDRESS])
    l->permAddressLength = attr_bytes(tb[A_IFLA_PERM_ADDRESS], l->permAddress, sizeof(l->permAddress));
  if (tb[A_IFLA_STATS64]) {
    parse_stats(&l->stats, tb[A_IFLA_STATS64], true);
    l->have |= NQ_NET_LINK_STATS | NQ_NET_LINK_STATS64;
  } else if (tb[A_IFLA_STATS]) {
    parse_stats(&l->stats, tb[A_IFLA_STATS], false);
    l->have |= NQ_NET_LINK_STATS;
  }
  if (tb[A_IFLA_CARRIER_CHANGES]) {
    l->carrierChanges = attr_u32(tb[A_IFLA_CARRIER_CHANGES]);
    l->have |= NQ_NET_LINK_CARRIER_CHANGES;
  }
  if (tb[A_IFLA_LINKINFO]) {
    struct rtattr* info[MAX_ATTR];
    parse_attrs(info, (struct rtattr*)RTA_DATA(tb[A_IFLA_LINKINFO]), (int)RTA_PAYLOAD(tb[A_IFLA_LINKINFO]));
    if (info[A_IFLA_INFO_KIND]) {
      attr_string(info[A_IFLA_INFO_KIND], l->kind, sizeof(l->kind));
      l->have |= NQ_NET_LINK_KIND;
    }
  }
  if (tb[A_IFLA_IFALIAS]) {
    attr_string(tb[A_IFLA_IFALIAS], l->alias, sizeof(l->alias));
    l->have |= NQ_NET_LINK_ALIAS;
  }
  if (tb[A_IFLA_PROMISCUITY]) {
    l->promiscuity = attr_u32(tb[A_IFLA_PROMISCUITY]);
    l->have |= NQ_NET_LINK_PROMISCUITY;
  }
  if (tb[A_IFLA_ALLMULTI]) {
    l->allmulti = attr_u32(tb[A_IFLA_ALLMULTI]);
    l->have |= NQ_NET_LINK_ALLMULTI;
  }
  if (tb[A_IFLA_MIN_MTU]) {
    l->minMtu = attr_u32(tb[A_IFLA_MIN_MTU]);
    l->have |= NQ_NET_LINK_MIN_MTU;
  }
  if (tb[A_IFLA_MAX_MTU]) {
    l->maxMtu = attr_u32(tb[A_IFLA_MAX_MTU]);
    l->have |= NQ_NET_LINK_MAX_MTU;
  }
  if (tb[A_IFLA_PROTO_DOWN]) {
    l->protoDown = attr_u8(tb[A_IFLA_PROTO_DOWN]);
    l->have |= NQ_NET_LINK_PROTO_DOWN;
  }
  return 0;
}

/* Runs a dump of `type` into a list of `size`-byte items */
static int dump_list(int type, size_t header, int family, MessageFn fn, size_t size, void** items, size_t* count)
{
  *items = NULL;
  *count = 0;
  Netlink nl;
  int ret = nl_open(&nl);
  if (ret != 0)
    return ret;
  Request r;
  uint8_t* msg = (uint8_t*)request_init(&r, type, header);
  msg[0] = (uint8_t)nl_family(family); /* every header starts with the family */
  List list = {NULL, 0, 0, size};
  ret = nl_dump(&nl, &r.n, fn, &list);
  nl_close(&nl);
  if (ret != 0) {
    NQFree(list.items);
    return ret;
  }
  *items = list.items;
  *count = list.count;
  return 0;
}

int NQNetGetLinks(NQNetLink** links, size_t* count)
{
  return dump_list(RTM_GETLINK, sizeof(struct ifinfomsg), NQ_NET_UNSPEC, parse_link, sizeof(NQNetLink),
                   (void**)links, count);
}

/* --- addresses ------------------------------------------------------------ */

static int parse_address(struct nlmsghdr* h, void* ctx)
{
  if (h->nlmsg_type != RTM_NEWADDR)
    return 0;
  struct ifaddrmsg* ifa = (struct ifaddrmsg*)NLMSG_DATA(h);
  int len = (int)h->nlmsg_len - NLMSG_LENGTH(sizeof(*ifa));
  int family = nq_family(ifa->ifa_family);
  if (len < 0 || family == NQ_NET_UNSPEC)
    return 0;
  struct rtattr* tb[MAX_ATTR];
  parse_attrs(tb, IFA_RTA(ifa), len);

  NQNetAddress* a = (NQNetAddress*)list_add((List*)ctx);
  if (a == NULL)
    return -NQ_ENOMEM;
  a->index = ifa->ifa_index;
  a->family = family;
  a->prefixLength = ifa->ifa_prefixlen;
  a->scope = ifa->ifa_scope;
  a->flags = tb[A_IFA_FLAGS] ? attr_u32(tb[A_IFA_FLAGS]) : ifa->ifa_flags;
  if (tb[A_IFA_LOCAL]) {
    attr_bytes(tb[A_IFA_LOCAL], a->local, 16);
    a->have |= NQ_NET_ADDR_LOCAL;
  }
  if (tb[A_IFA_ADDRESS]) {
    attr_bytes(tb[A_IFA_ADDRESS], a->address, 16);
    a->have |= NQ_NET_ADDR_ADDRESS;
  }
  if (tb[A_IFA_BROADCAST]) {
    attr_bytes(tb[A_IFA_BROADCAST], a->broadcast, 16);
    a->have |= NQ_NET_ADDR_BROADCAST;
  }
  if (tb[A_IFA_ANYCAST]) {
    attr_bytes(tb[A_IFA_ANYCAST], a->anycast, 16);
    a->have |= NQ_NET_ADDR_ANYCAST;
  }
  if (tb[A_IFA_LABEL]) {
    attr_string(tb[A_IFA_LABEL], a->label, sizeof(a->label));
    a->have |= NQ_NET_ADDR_LABEL;
  }
  if (tb[A_IFA_CACHEINFO] && RTA_PAYLOAD(tb[A_IFA_CACHEINFO]) >= sizeof(struct ifa_cacheinfo)) {
    const struct ifa_cacheinfo* ci = (const struct ifa_cacheinfo*)RTA_DATA(tb[A_IFA_CACHEINFO]);
    a->preferredLifetime = ci->ifa_prefered;
    a->validLifetime = ci->ifa_valid;
    a->have |= NQ_NET_ADDR_LIFETIME;
  }
  if (tb[A_IFA_RT_PRIORITY]) {
    a->metric = attr_u32(tb[A_IFA_RT_PRIORITY]);
    a->have |= NQ_NET_ADDR_METRIC;
  }
  return 0;
}

int NQNetGetAddresses(int family, NQNetAddress** addresses, size_t* count)
{
  return dump_list(RTM_GETADDR, sizeof(struct ifaddrmsg), family, parse_address, sizeof(NQNetAddress),
                   (void**)addresses, count);
}

int NQNetAddressChange(NQNetChange change, const NQNetAddress* a)
{
  Request r;
  int type = change == NQ_NET_DELETE ? RTM_DELADDR : RTM_NEWADDR;
  struct ifaddrmsg* ifa = (struct ifaddrmsg*)request_init(&r, type, sizeof(struct ifaddrmsg));
  r.n.nlmsg_flags = change_flags(change);
  size_t bytes = family_bytes(a->family);
  ifa->ifa_family = (uint8_t)nl_family(a->family);
  ifa->ifa_prefixlen = a->prefixLength;
  ifa->ifa_scope = a->scope;
  ifa->ifa_index = a->index;
  if (a->flags <= 0xFF)
    ifa->ifa_flags = (uint8_t)a->flags;
  if (a->have & NQ_NET_ADDR_LOCAL)
    add_attr(&r.n, A_IFA_LOCAL, a->local, bytes);
  if (a->have & NQ_NET_ADDR_ADDRESS)
    add_attr(&r.n, A_IFA_ADDRESS, a->address, bytes);
  if (a->have & NQ_NET_ADDR_BROADCAST)
    add_attr(&r.n, A_IFA_BROADCAST, a->broadcast, bytes);
  if (a->have & NQ_NET_ADDR_ANYCAST)
    add_attr(&r.n, A_IFA_ANYCAST, a->anycast, bytes);
  if (a->have & NQ_NET_ADDR_LABEL)
    add_string(&r.n, A_IFA_LABEL, a->label, true);
  if (a->have & NQ_NET_ADDR_METRIC)
    add_u32(&r.n, A_IFA_RT_PRIORITY, a->metric);
  if (a->have & NQ_NET_ADDR_LIFETIME) {
    struct ifa_cacheinfo ci;
    NQMemset(&ci, 0, sizeof(ci));
    ci.ifa_prefered = a->preferredLifetime;
    ci.ifa_valid = a->validLifetime;
    add_attr(&r.n, A_IFA_CACHEINFO, &ci, sizeof(ci));
  }
  if (a->flags > 0xFF)
    add_u32(&r.n, A_IFA_FLAGS, a->flags);
  return run_request(&r);
}

/* --- routes --------------------------------------------------------------- */

static void parse_via(NQNetRoute* route, const struct rtattr* a)
{
  size_t len = RTA_PAYLOAD(a);
  if (len < 2)
    return;
  const uint8_t* p = (const uint8_t*)RTA_DATA(a);
  uint16_t family;
  NQMemcpy(&family, p, 2);
  route->viaFamily = nq_family(family);
  len -= 2;
  NQMemcpy(route->via, p + 2, len < 16 ? len : 16);
  route->have |= NQ_NET_ROUTE_VIA;
}

static int parse_multipath(NQNetRoute* route, const struct rtattr* a)
{
  const uint8_t* p = (const uint8_t*)RTA_DATA(a);
  size_t len = RTA_PAYLOAD(a);
  List list = {NULL, 0, 0, sizeof(NQNetNexthop)};
  while (len >= sizeof(struct rtnexthop)) {
    const struct rtnexthop* nh = (const struct rtnexthop*)p;
    if (nh->rtnh_len < sizeof(*nh) || nh->rtnh_len > len)
      break;
    NQNetNexthop* hop = (NQNetNexthop*)list_add(&list);
    if (hop == NULL) {
      NQFree(list.items);
      return -NQ_ENOMEM;
    }
    hop->index = (unsigned)nh->rtnh_ifindex;
    hop->hops = nh->rtnh_hops;
    hop->flags = nh->rtnh_flags;
    if (nh->rtnh_len > sizeof(*nh)) {
      struct rtattr* tb[MAX_ATTR];
      parse_attrs(tb, RTNH_DATA(nh), nh->rtnh_len - (int)sizeof(*nh));
      if (tb[A_RTA_GATEWAY]) {
        hop->gatewayFamily = route->family;
        attr_bytes(tb[A_RTA_GATEWAY], hop->gateway, 16);
      }
    }
    size_t step = RTNH_ALIGN(nh->rtnh_len);
    if (step > len)
      break;
    p += step;
    len -= step;
  }
  route->nexthops = (NQNetNexthop*)list.items;
  route->nexthopCount = list.count;
  return 0;
}

/* Fills `route` from a RTM_NEWROUTE message */
static int fill_route(NQNetRoute* route, struct nlmsghdr* h)
{
  struct rtmsg* r = (struct rtmsg*)NLMSG_DATA(h);
  int len = (int)h->nlmsg_len - NLMSG_LENGTH(sizeof(*r));
  struct rtattr* tb[MAX_ATTR];
  parse_attrs(tb, RTM_RTA(r), len);

  NQMemset(route, 0, sizeof(*route));
  route->family = nq_family(r->rtm_family);
  route->dstLength = r->rtm_dst_len;
  route->srcLength = r->rtm_src_len;
  route->tos = r->rtm_tos;
  route->protocol = r->rtm_protocol;
  route->scope = r->rtm_scope;
  route->type = r->rtm_type;
  route->flags = r->rtm_flags;
  route->table = tb[A_RTA_TABLE] ? attr_u32(tb[A_RTA_TABLE]) : r->rtm_table;
  if (tb[A_RTA_DST]) {
    attr_bytes(tb[A_RTA_DST], route->dst, 16);
    route->have |= NQ_NET_ROUTE_DST;
  }
  if (tb[A_RTA_SRC]) {
    attr_bytes(tb[A_RTA_SRC], route->src, 16);
    route->have |= NQ_NET_ROUTE_SRC;
  }
  if (tb[A_RTA_GATEWAY]) {
    attr_bytes(tb[A_RTA_GATEWAY], route->gateway, 16);
    route->have |= NQ_NET_ROUTE_GATEWAY;
  }
  if (tb[A_RTA_VIA])
    parse_via(route, tb[A_RTA_VIA]);
  if (tb[A_RTA_PREFSRC]) {
    attr_bytes(tb[A_RTA_PREFSRC], route->prefsrc, 16);
    route->have |= NQ_NET_ROUTE_PREFSRC;
  }
  if (tb[A_RTA_OIF]) {
    route->oif = attr_u32(tb[A_RTA_OIF]);
    route->have |= NQ_NET_ROUTE_OIF;
  }
  if (tb[A_RTA_IIF]) {
    route->iif = attr_u32(tb[A_RTA_IIF]);
    route->have |= NQ_NET_ROUTE_IIF;
  }
  if (tb[A_RTA_PRIORITY]) {
    route->priority = attr_u32(tb[A_RTA_PRIORITY]);
    route->have |= NQ_NET_ROUTE_PRIORITY;
  }
  if (tb[A_RTA_MARK]) {
    route->mark = attr_u32(tb[A_RTA_MARK]);
    route->have |= NQ_NET_ROUTE_MARK;
  }
  if (tb[A_RTA_UID]) {
    route->uid = attr_u32(tb[A_RTA_UID]);
    route->have |= NQ_NET_ROUTE_UID;
  }
  if (tb[A_RTA_FLOW]) {
    route->flow = attr_u32(tb[A_RTA_FLOW]);
    route->have |= NQ_NET_ROUTE_FLOW;
  }
  if (tb[A_RTA_NH_ID]) {
    route->nhid = attr_u32(tb[A_RTA_NH_ID]);
    route->have |= NQ_NET_ROUTE_NHID;
  }
  if (tb[A_RTA_PREF]) {
    route->pref = attr_u8(tb[A_RTA_PREF]);
    route->have |= NQ_NET_ROUTE_PREF;
  }
  if (tb[A_RTA_CACHEINFO] && RTA_PAYLOAD(tb[A_RTA_CACHEINFO]) >= sizeof(struct rta_cacheinfo)) {
    const struct rta_cacheinfo* ci = (const struct rta_cacheinfo*)RTA_DATA(tb[A_RTA_CACHEINFO]);
    long hz = user_hz();
    route->expires = (int32_t)(ci->rta_expires / hz);
    route->error = ci->rta_error;
    route->users = ci->rta_clntref;
    route->used = ci->rta_used;
    route->age = (uint32_t)(ci->rta_lastuse / hz);
    route->ipid = ci->rta_id;
    route->ts = ci->rta_ts;
    route->tsage = ci->rta_tsage;
    route->have |= NQ_NET_ROUTE_CACHEINFO;
  }
  if (tb[A_RTA_METRICS]) {
    struct rtattr* mx[MAX_ATTR];
    parse_attrs(mx, (struct rtattr*)RTA_DATA(tb[A_RTA_METRICS]), (int)RTA_PAYLOAD(tb[A_RTA_METRICS]));
    for (int i = 1; i <= NQ_NET_RTAX_MAX; i++) {
      if (mx[i] == NULL)
        continue;
      if (i == NQ_NET_RTAX_LOCK) {
        route->metricsLock = attr_u32(mx[i]);
      } else if (i == NQ_NET_RTAX_CC_ALGO) {
        attr_string(mx[i], route->congestion, sizeof(route->congestion));
        route->metricsHave |= 1u << i;
      } else {
        route->metrics[i] = attr_u32(mx[i]);
        route->metricsHave |= 1u << i;
      }
    }
    route->have |= NQ_NET_ROUTE_METRICS;
  }
  if (tb[A_RTA_MULTIPATH])
    return parse_multipath(route, tb[A_RTA_MULTIPATH]);
  return 0;
}

static int parse_route(struct nlmsghdr* h, void* ctx)
{
  if (h->nlmsg_type != RTM_NEWROUTE)
    return 0;
  struct rtmsg* r = (struct rtmsg*)NLMSG_DATA(h);
  if (h->nlmsg_len < NLMSG_LENGTH(sizeof(*r)) || nq_family(r->rtm_family) == NQ_NET_UNSPEC)
    return 0;
  NQNetRoute* route = (NQNetRoute*)list_add((List*)ctx);
  if (route == NULL)
    return -NQ_ENOMEM;
  return fill_route(route, h);
}

int NQNetGetRoutes(int family, NQNetRoute** routes, size_t* count)
{
  int ret = dump_list(RTM_GETROUTE, sizeof(struct rtmsg), family, parse_route, sizeof(NQNetRoute),
                      (void**)routes, count);
  return ret;
}

static int keep_answer(struct nlmsghdr* h, void* ctx)
{
  NQNetRoute** found = (NQNetRoute**)ctx;
  if (h->nlmsg_type != RTM_NEWROUTE || *found != NULL)
    return 0;
  NQNetRoute* route = (NQNetRoute*)NQMalloc(sizeof(NQNetRoute));
  if (route == NULL)
    return -NQ_ENOMEM;
  int ret = fill_route(route, h);
  if (ret != 0) {
    NQFree(route);
    return ret;
  }
  *found = route;
  return 0;
}

int NQNetRouteGet(const NQNetRouteQuery* q, NQNetRoute** route)
{
  *route = NULL;
  Request r;
  struct rtmsg* rtm = (struct rtmsg*)request_init(&r, RTM_GETROUTE, sizeof(struct rtmsg));
  int family = q->family ? q->family : NQ_NET_INET;
  size_t bytes = family_bytes(family);
  rtm->rtm_family = (uint8_t)nl_family(family);
  rtm->rtm_dst_len = q->dstLength;
  rtm->rtm_tos = q->tos;
  if (q->dstLength)
    add_attr(&r.n, A_RTA_DST, q->dst, bytes);
  if (q->have & NQ_NET_QUERY_SRC) {
    rtm->rtm_src_len = q->srcLength;
    if (q->srcLength)
      add_attr(&r.n, A_RTA_SRC, q->src, bytes);
  }
  if (q->have & NQ_NET_QUERY_UID)
    add_u32(&r.n, A_RTA_UID, q->uid);
  if (q->have & NQ_NET_QUERY_IIF)
    add_u32(&r.n, A_RTA_IIF, q->iif);
  if (q->have & NQ_NET_QUERY_OIF)
    add_u32(&r.n, A_RTA_OIF, q->oif);
  if (q->have & NQ_NET_QUERY_MARK)
    add_u32(&r.n, A_RTA_MARK, q->mark);
  if (family == NQ_NET_INET)
    rtm->rtm_flags |= A_RTM_F_LOOKUP_TABLE;
  if (q->have & NQ_NET_QUERY_FIBMATCH)
    rtm->rtm_flags |= A_RTM_F_FIB_MATCH;

  Netlink nl;
  int ret = nl_open(&nl);
  if (ret != 0)
    return ret;
  ret = nl_talk(&nl, &r.n, keep_answer, route);
  nl_close(&nl);
  if (ret != 0) {
    NQNetFreeRoutes(*route, *route ? 1 : 0);
    *route = NULL;
    return ret;
  }
  return *route ? 0 : -NQ_EIO;
}

int NQNetRouteChange(NQNetChange change, const NQNetRoute* route)
{
  Request r;
  int type = change == NQ_NET_DELETE ? RTM_DELROUTE : RTM_NEWROUTE;
  struct rtmsg* rtm = (struct rtmsg*)request_init(&r, type, sizeof(struct rtmsg));
  r.n.nlmsg_flags = change_flags(change);
  size_t bytes = family_bytes(route->family);
  rtm->rtm_family = (uint8_t)nl_family(route->family);
  rtm->rtm_dst_len = route->dstLength;
  rtm->rtm_src_len = route->srcLength;
  rtm->rtm_tos = route->tos;
  rtm->rtm_protocol = route->protocol;
  rtm->rtm_scope = route->scope;
  rtm->rtm_type = route->type;
  rtm->rtm_flags = route->flags;
  if (route->table < 256) {
    rtm->rtm_table = (uint8_t)route->table;
  } else {
    rtm->rtm_table = RT_TABLE_UNSPEC;
    add_u32(&r.n, A_RTA_TABLE, route->table);
  }
  if (route->have & NQ_NET_ROUTE_PREFSRC)
    add_attr(&r.n, A_RTA_PREFSRC, route->prefsrc, bytes);
  if (route->have & NQ_NET_ROUTE_GATEWAY)
    add_attr(&r.n, A_RTA_GATEWAY, route->gateway, bytes);
  if (route->have & NQ_NET_ROUTE_VIA) {
    uint8_t via[18];
    uint16_t family = (uint16_t)nl_family(route->viaFamily);
    size_t len = family_bytes(route->viaFamily);
    NQMemcpy(via, &family, 2);
    NQMemcpy(via + 2, route->via, len);
    add_attr(&r.n, A_RTA_VIA, via, len + 2);
  }
  if (route->have & NQ_NET_ROUTE_SRC)
    add_attr(&r.n, A_RTA_SRC, route->src, bytes);
  if (route->have & NQ_NET_ROUTE_PRIORITY)
    add_u32(&r.n, A_RTA_PRIORITY, route->priority);
  if (route->have & NQ_NET_ROUTE_EXPIRES)
    add_u32(&r.n, A_RTA_EXPIRES, (uint32_t)route->expires);
  if (route->have & NQ_NET_ROUTE_PREF)
    add_u8(&r.n, A_RTA_PREF, route->pref);
  if (route->have & NQ_NET_ROUTE_DST)
    add_attr(&r.n, A_RTA_DST, route->dst, bytes);
  if (route->have & NQ_NET_ROUTE_OIF)
    add_u32(&r.n, A_RTA_OIF, route->oif);
  if ((route->have & NQ_NET_ROUTE_METRICS) && (route->metricsHave || route->metricsLock)) {
    struct rtattr* mx = nest_begin(&r.n, A_RTA_METRICS);
    for (int i = 2; i <= NQ_NET_RTAX_MAX; i++) {
      if (!(route->metricsHave & (1u << i)))
        continue;
      if (i == NQ_NET_RTAX_CC_ALGO)
        add_string(&r.n, i, route->congestion, true);
      else
        add_u32(&r.n, i, route->metrics[i]);
    }
    if (route->metricsLock)
      add_u32(&r.n, NQ_NET_RTAX_LOCK, route->metricsLock);
    nest_end(&r.n, mx);
  }
  if (route->nexthopCount > 0) {
    struct rtattr* mp = nest_begin(&r.n, A_RTA_MULTIPATH);
    for (size_t i = 0; i < route->nexthopCount; i++) {
      const NQNetNexthop* hop = &route->nexthops[i];
      struct rtattr* start = add_attr(&r.n, 0, NULL, sizeof(struct rtnexthop) - sizeof(struct rtattr));
      if (start == NULL)
        break;
      struct rtnexthop* nh = (struct rtnexthop*)start;
      nh->rtnh_flags = (unsigned char)hop->flags;
      nh->rtnh_hops = hop->hops;
      nh->rtnh_ifindex = (int)hop->index;
      if (hop->gatewayFamily == route->family)
        add_attr(&r.n, A_RTA_GATEWAY, hop->gateway, bytes);
      nh->rtnh_len = (unsigned short)((char*)&r.n + NLMSG_ALIGN(r.n.nlmsg_len) - (char*)nh);
    }
    nest_end(&r.n, mp);
  }
  return run_request(&r);
}

/* --- neighbours ----------------------------------------------------------- */

static int parse_neighbor(struct nlmsghdr* h, void* ctx)
{
  if (h->nlmsg_type != RTM_NEWNEIGH)
    return 0;
  struct nq_ndmsg* ndm = (struct nq_ndmsg*)NLMSG_DATA(h);
  int len = (int)h->nlmsg_len - NLMSG_LENGTH(sizeof(*ndm));
  int family = nq_family(ndm->ndm_family);
  if (len < 0 || family == NQ_NET_UNSPEC)
    return 0;
  struct rtattr* tb[MAX_ATTR];
  parse_attrs(tb, (struct rtattr*)((char*)ndm + NLMSG_ALIGN(sizeof(*ndm))), len);

  NQNetNeighbor* n = (NQNetNeighbor*)list_add((List*)ctx);
  if (n == NULL)
    return -NQ_ENOMEM;
  n->family = family;
  n->index = (unsigned)ndm->ndm_ifindex;
  n->state = ndm->ndm_state;
  n->flags = ndm->ndm_flags;
  if (tb[A_NDA_DST]) {
    attr_bytes(tb[A_NDA_DST], n->dst, 16);
    n->have |= NQ_NET_NEIGH_DST;
  }
  if (tb[A_NDA_LLADDR]) {
    n->lladdrLength = attr_bytes(tb[A_NDA_LLADDR], n->lladdr, sizeof(n->lladdr));
    n->have |= NQ_NET_NEIGH_LLADDR;
  }
  if (tb[A_NDA_CACHEINFO] && RTA_PAYLOAD(tb[A_NDA_CACHEINFO]) >= sizeof(struct nda_cacheinfo)) {
    const struct nda_cacheinfo* ci = (const struct nda_cacheinfo*)RTA_DATA(tb[A_NDA_CACHEINFO]);
    long hz = user_hz();
    n->refcnt = ci->ndm_refcnt;
    n->used = (uint32_t)(ci->ndm_used / hz);
    n->confirmed = (uint32_t)(ci->ndm_confirmed / hz);
    n->updated = (uint32_t)(ci->ndm_updated / hz);
    n->have |= NQ_NET_NEIGH_CACHEINFO;
  }
  if (tb[A_NDA_PROBES]) {
    n->probes = attr_u32(tb[A_NDA_PROBES]);
    n->have |= NQ_NET_NEIGH_PROBES;
  }
  if (tb[A_NDA_PROTOCOL]) {
    n->protocol = attr_u8(tb[A_NDA_PROTOCOL]);
    n->have |= NQ_NET_NEIGH_PROTOCOL;
  }
  if (tb[A_NDA_FLAGS_EXT]) {
    n->extFlags = attr_u32(tb[A_NDA_FLAGS_EXT]);
    n->have |= NQ_NET_NEIGH_EXT_FLAGS;
  }
  return 0;
}

int NQNetGetNeighbors(int family, NQNetNeighbor** neighbors, size_t* count)
{
  return dump_list(RTM_GETNEIGH, sizeof(struct nq_ndmsg), family, parse_neighbor, sizeof(NQNetNeighbor),
                   (void**)neighbors, count);
}

int NQNetNeighborChange(NQNetChange change, const NQNetNeighbor* n)
{
  Request r;
  int type = change == NQ_NET_DELETE ? RTM_DELNEIGH : RTM_NEWNEIGH;
  struct nq_ndmsg* ndm = (struct nq_ndmsg*)request_init(&r, type, sizeof(struct nq_ndmsg));
  r.n.nlmsg_flags = change_flags(change);
  ndm->ndm_family = (uint8_t)nl_family(n->family);
  ndm->ndm_ifindex = (int32_t)n->index;
  ndm->ndm_state = n->state;
  ndm->ndm_flags = n->flags;
  if (n->have & NQ_NET_NEIGH_PROTOCOL)
    add_u8(&r.n, A_NDA_PROTOCOL, n->protocol);
  add_attr(&r.n, A_NDA_DST, n->dst, family_bytes(n->family));
  if (n->have & NQ_NET_NEIGH_EXT_FLAGS)
    add_u32(&r.n, A_NDA_FLAGS_EXT, n->extFlags);
  if (n->have & NQ_NET_NEIGH_LLADDR)
    add_attr(&r.n, A_NDA_LLADDR, n->lladdr, n->lladdrLength);
  return run_request(&r);
}

/* --- changing links ------------------------------------------------------- */

int NQNetLinkChange(NQNetChange change, const NQNetLinkRequest* q)
{
  Request r;
  int type = change == NQ_NET_DELETE ? RTM_DELLINK : RTM_NEWLINK;
  struct ifinfomsg* ifi = (struct ifinfomsg*)request_init(&r, type, sizeof(struct ifinfomsg));
  r.n.nlmsg_flags = change == NQ_NET_CHANGE ? 0 : change_flags(change);
  ifi->ifi_family = AF_UNSPEC;
  ifi->ifi_index = (int)q->index;
  ifi->ifi_flags = q->flags;
  ifi->ifi_change = q->flagsMask;
  if (q->set & NQ_NET_SET_ADDRESS)
    add_attr(&r.n, A_IFLA_ADDRESS, q->address, q->addressLength);
  if (q->set & NQ_NET_SET_BROADCAST)
    add_attr(&r.n, A_IFLA_BROADCAST, q->broadcast, q->broadcastLength);
  if (q->set & NQ_NET_SET_TXQLEN)
    add_u32(&r.n, A_IFLA_TXQLEN, q->txqlen);
  if (q->set & NQ_NET_SET_MTU)
    add_u32(&r.n, A_IFLA_MTU, q->mtu);
  if (q->set & NQ_NET_SET_MASTER)
    add_u32(&r.n, A_IFLA_MASTER, q->master);
  if (q->set & NQ_NET_SET_ALIAS)
    add_string(&r.n, A_IFLA_IFALIAS, q->alias, false);
  if (q->set & NQ_NET_SET_GROUP)
    add_u32(&r.n, A_IFLA_GROUP, q->group);
  if (q->set & NQ_NET_SET_LINKMODE)
    add_u8(&r.n, A_IFLA_LINKMODE, q->linkMode);
  if (q->set & NQ_NET_SET_OPERSTATE)
    add_u8(&r.n, A_IFLA_OPERSTATE, q->operState);
  if (q->set & NQ_NET_SET_CARRIER)
    add_u8(&r.n, A_IFLA_CARRIER, q->carrier);
  if (q->set & NQ_NET_SET_PROTODOWN)
    add_u8(&r.n, A_IFLA_PROTO_DOWN, q->protoDown);
  if (q->set & NQ_NET_SET_LINK)
    add_u32(&r.n, A_IFLA_LINK, q->link);
  if (q->set & NQ_NET_SET_NAME)
    add_string(&r.n, A_IFLA_IFNAME, q->name, true);
  if (q->set & NQ_NET_SET_KIND) {
    struct rtattr* info = nest_begin(&r.n, A_IFLA_LINKINFO);
    add_string(&r.n, A_IFLA_INFO_KIND, q->kind, false);
    if (q->set & NQ_NET_SET_PEER) {
      struct rtattr* data = nest_begin(&r.n, A_IFLA_INFO_DATA);
      struct rtattr* peer = nest_begin(&r.n, A_VETH_INFO_PEER);
      struct ifinfomsg blank;
      NQMemset(&blank, 0, sizeof(blank));
      NQMemcpy((char*)&r.n + NLMSG_ALIGN(r.n.nlmsg_len), &blank, sizeof(blank));
      r.n.nlmsg_len = NLMSG_ALIGN(r.n.nlmsg_len) + NLMSG_ALIGN(sizeof(blank));
      add_string(&r.n, A_IFLA_IFNAME, q->peer, true);
      nest_end(&r.n, peer);
      nest_end(&r.n, data);
    }
    nest_end(&r.n, info);
  }
  return run_request(&r);
}

#endif /* NQ_OS_LINUX */
