/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_NET_NETCONFIG_H
#define _LIBNETQ_NET_NETCONFIG_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The network configuration of the system: interfaces (links), their
 * addresses, the routing tables and the neighbour (ARP / NDP) cache, as
 * the "ip" tool shows them.
 *
 * Linux and Android read and change it over rtnetlink. macOS reads it
 * with getifaddrs and the routing sysctls, Windows with the IP Helper
 * API; there the changes return -NQ_ENOTSUPP.
 *
 * Numbers follow Linux on every system, so that they mean the same
 * everywhere: interface flags are IFF_*, link types ARPHRD_*, operational
 * states IF_OPER_*, address flags IFA_F_*, scopes RT_SCOPE_*, route types
 * RTN_*, protocols RTPROT_*, tables RT_TABLE_* and neighbour states
 * NUD_*. Addresses are in network byte order: 4 bytes for IPv4, 16 for
 * IPv6. Errors are negative libnetq error codes.
 */

enum {
  NQ_NET_UNSPEC = 0,
  NQ_NET_INET = 4,
  NQ_NET_INET6 = 6,
};

/* Interface flags (Linux's IFF_*) */
enum {
  NQ_NET_IFF_UP = 0x1,
  NQ_NET_IFF_BROADCAST = 0x2,
  NQ_NET_IFF_DEBUG = 0x4,
  NQ_NET_IFF_LOOPBACK = 0x8,
  NQ_NET_IFF_POINTOPOINT = 0x10,
  NQ_NET_IFF_NOTRAILERS = 0x20,
  NQ_NET_IFF_RUNNING = 0x40,
  NQ_NET_IFF_NOARP = 0x80,
  NQ_NET_IFF_PROMISC = 0x100,
  NQ_NET_IFF_ALLMULTI = 0x200,
  NQ_NET_IFF_MASTER = 0x400,
  NQ_NET_IFF_SLAVE = 0x800,
  NQ_NET_IFF_MULTICAST = 0x1000,
  NQ_NET_IFF_PORTSEL = 0x2000,
  NQ_NET_IFF_AUTOMEDIA = 0x4000,
  NQ_NET_IFF_DYNAMIC = 0x8000,
  NQ_NET_IFF_LOWER_UP = 0x10000,
  NQ_NET_IFF_DORMANT = 0x20000,
  NQ_NET_IFF_ECHO = 0x40000,
};

/* Link types (ARPHRD_*) the other systems map to */
enum {
  NQ_NET_ARPHRD_ETHER = 1,
  NQ_NET_ARPHRD_PPP = 512,
  NQ_NET_ARPHRD_TUNNEL = 768,
  NQ_NET_ARPHRD_TUNNEL6 = 769,
  NQ_NET_ARPHRD_LOOPBACK = 772,
  NQ_NET_ARPHRD_IEEE80211 = 801,
  NQ_NET_ARPHRD_NONE = 0xFFFE,
};

/* Operational states (IF_OPER_*) */
enum {
  NQ_NET_OPER_UNKNOWN = 0,
  NQ_NET_OPER_NOTPRESENT = 1,
  NQ_NET_OPER_DOWN = 2,
  NQ_NET_OPER_LOWERLAYERDOWN = 3,
  NQ_NET_OPER_TESTING = 4,
  NQ_NET_OPER_DORMANT = 5,
  NQ_NET_OPER_UP = 6,
};

/* Address flags (IFA_F_*) */
enum {
  NQ_NET_IFA_SECONDARY = 0x01, /* "temporary" for IPv6 */
  NQ_NET_IFA_NODAD = 0x02,
  NQ_NET_IFA_OPTIMISTIC = 0x04,
  NQ_NET_IFA_DADFAILED = 0x08,
  NQ_NET_IFA_HOMEADDRESS = 0x10,
  NQ_NET_IFA_DEPRECATED = 0x20,
  NQ_NET_IFA_TENTATIVE = 0x40,
  NQ_NET_IFA_PERMANENT = 0x80,
  NQ_NET_IFA_MANAGETEMPADDR = 0x100,
  NQ_NET_IFA_NOPREFIXROUTE = 0x200,
  NQ_NET_IFA_MCAUTOJOIN = 0x400,
  NQ_NET_IFA_STABLE_PRIVACY = 0x800,
};

#define NQ_NET_LIFETIME_INFINITE 0xFFFFFFFFu

/* Scopes (RT_SCOPE_*) */
enum {
  NQ_NET_SCOPE_UNIVERSE = 0,
  NQ_NET_SCOPE_SITE = 200,
  NQ_NET_SCOPE_LINK = 253,
  NQ_NET_SCOPE_HOST = 254,
  NQ_NET_SCOPE_NOWHERE = 255,
};

/* Route types (RTN_*) */
enum {
  NQ_NET_RTN_UNSPEC = 0,
  NQ_NET_RTN_UNICAST = 1,
  NQ_NET_RTN_LOCAL = 2,
  NQ_NET_RTN_BROADCAST = 3,
  NQ_NET_RTN_ANYCAST = 4,
  NQ_NET_RTN_MULTICAST = 5,
  NQ_NET_RTN_BLACKHOLE = 6,
  NQ_NET_RTN_UNREACHABLE = 7,
  NQ_NET_RTN_PROHIBIT = 8,
  NQ_NET_RTN_THROW = 9,
  NQ_NET_RTN_NAT = 10,
  NQ_NET_RTN_XRESOLVE = 11,
};

/* Route protocols (RTPROT_*) the other systems map to */
enum {
  NQ_NET_RTPROT_UNSPEC = 0,
  NQ_NET_RTPROT_REDIRECT = 1,
  NQ_NET_RTPROT_KERNEL = 2,
  NQ_NET_RTPROT_BOOT = 3,
  NQ_NET_RTPROT_STATIC = 4,
  NQ_NET_RTPROT_RA = 9,
  NQ_NET_RTPROT_DHCP = 16,
};

/* Routing tables (RT_TABLE_*) */
enum {
  NQ_NET_TABLE_UNSPEC = 0,
  NQ_NET_TABLE_DEFAULT = 253,
  NQ_NET_TABLE_MAIN = 254,
  NQ_NET_TABLE_LOCAL = 255,
};

/* Route flags: Linux's RTNH_F_* and RTM_F_*, with the RTCF_* cache flags
 * of a cloned route in the high bits */
enum {
  NQ_NET_RTNH_F_DEAD = 0x1,
  NQ_NET_RTNH_F_PERVASIVE = 0x2,
  NQ_NET_RTNH_F_ONLINK = 0x4,
  NQ_NET_RTNH_F_OFFLOAD = 0x8,
  NQ_NET_RTNH_F_LINKDOWN = 0x10,
  NQ_NET_RTNH_F_UNRESOLVED = 0x20,
  NQ_NET_RTNH_F_TRAP = 0x40,
  NQ_NET_RTM_F_NOTIFY = 0x100,
  NQ_NET_RTM_F_CLONED = 0x200,
  NQ_NET_RTM_F_OFFLOAD = 0x4000,
  NQ_NET_RTM_F_TRAP = 0x8000,
  NQ_NET_RTM_F_OFFLOAD_FAILED = 0x20000000,
};

/* Neighbour states (NUD_*) and flags (NTF_*) */
enum {
  NQ_NET_NUD_INCOMPLETE = 0x01,
  NQ_NET_NUD_REACHABLE = 0x02,
  NQ_NET_NUD_STALE = 0x04,
  NQ_NET_NUD_DELAY = 0x08,
  NQ_NET_NUD_PROBE = 0x10,
  NQ_NET_NUD_FAILED = 0x20,
  NQ_NET_NUD_NOARP = 0x40,
  NQ_NET_NUD_PERMANENT = 0x80,
  NQ_NET_NUD_NONE = 0x00,
};
enum {
  NQ_NET_NTF_USE = 0x01,
  NQ_NET_NTF_SELF = 0x02,
  NQ_NET_NTF_MASTER = 0x04,
  NQ_NET_NTF_PROXY = 0x08,
  NQ_NET_NTF_EXT_LEARNED = 0x10,
  NQ_NET_NTF_OFFLOADED = 0x20,
  NQ_NET_NTF_ROUTER = 0x80,
};
#define NQ_NET_NTF_EXT_MANAGED 0x1u

#define NQ_NET_NAME_SIZE 64
#define NQ_NET_HWADDR_SIZE 32

typedef struct NQNetLinkStats {
  uint64_t rxPackets, txPackets, rxBytes, txBytes;
  uint64_t rxErrors, txErrors, rxDropped, txDropped;
  uint64_t multicast, collisions;
  uint64_t rxLengthErrors, rxOverErrors, rxCrcErrors, rxFrameErrors, rxFifoErrors, rxMissedErrors;
  uint64_t txAbortedErrors, txCarrierErrors, txFifoErrors, txHeartbeatErrors, txWindowErrors;
  uint64_t rxCompressed, txCompressed, rxNohandler, rxOtherhostDropped;
} NQNetLinkStats;

/* What a NQNetLink has: its `have` bits */
enum {
  NQ_NET_LINK_MTU = 1 << 0,
  NQ_NET_LINK_QDISC = 1 << 1,
  NQ_NET_LINK_MASTER = 1 << 2,
  NQ_NET_LINK_LINK = 1 << 3,    /* `link`, 0 for none ("@NONE") */
  NQ_NET_LINK_NETNSID = 1 << 4, /* `link` is in another namespace */
  NQ_NET_LINK_OPERSTATE = 1 << 5,
  NQ_NET_LINK_LINKMODE = 1 << 6,
  NQ_NET_LINK_GROUP = 1 << 7,
  NQ_NET_LINK_TXQLEN = 1 << 8,
  NQ_NET_LINK_STATS = 1 << 9,
  NQ_NET_LINK_STATS64 = 1 << 10, /* the counters are 64-bit ones */
  NQ_NET_LINK_CARRIER_CHANGES = 1 << 11,
  NQ_NET_LINK_KIND = 1 << 12,
  NQ_NET_LINK_ALIAS = 1 << 13,
  NQ_NET_LINK_PROMISCUITY = 1 << 14,
  NQ_NET_LINK_ALLMULTI = 1 << 15,
  NQ_NET_LINK_MIN_MTU = 1 << 16,
  NQ_NET_LINK_MAX_MTU = 1 << 17,
  NQ_NET_LINK_PROTO_DOWN = 1 << 18,
};

typedef struct NQNetLink {
  unsigned index;
  char name[NQ_NET_NAME_SIZE];
  unsigned flags; /* NQ_NET_IFF_* */
  unsigned type;  /* ARPHRD_* */
  uint32_t have;  /* NQ_NET_LINK_* */
  unsigned mtu;
  char qdisc[32];
  unsigned master;
  unsigned link;
  int linkNetnsid;
  uint8_t operState;
  uint8_t linkMode;
  uint8_t protoDown;
  uint32_t group;
  uint32_t txqlen;
  uint8_t address[NQ_NET_HWADDR_SIZE];
  size_t addressLength;
  uint8_t broadcast[NQ_NET_HWADDR_SIZE];
  size_t broadcastLength;
  uint8_t permAddress[NQ_NET_HWADDR_SIZE];
  size_t permAddressLength;
  NQNetLinkStats stats;
  uint32_t carrierChanges;
  char kind[32];
  char alias[256];
  uint32_t promiscuity, allmulti, minMtu, maxMtu;
} NQNetLink;

/* What a NQNetAddress has: its `have` bits */
enum {
  NQ_NET_ADDR_LOCAL = 1 << 0,
  NQ_NET_ADDR_ADDRESS = 1 << 1, /* the peer, or the same as local */
  NQ_NET_ADDR_BROADCAST = 1 << 2,
  NQ_NET_ADDR_ANYCAST = 1 << 3,
  NQ_NET_ADDR_LABEL = 1 << 4,
  NQ_NET_ADDR_LIFETIME = 1 << 5,
  NQ_NET_ADDR_METRIC = 1 << 6,
  /* broadcast "+" and "-": all ones or all zeros after the prefix */
  NQ_NET_ADDR_BROADCAST_ONES = 1 << 7,
  NQ_NET_ADDR_BROADCAST_ZEROS = 1 << 8,
};

typedef struct NQNetAddress {
  unsigned index;
  int family; /* NQ_NET_INET or NQ_NET_INET6 */
  uint8_t prefixLength;
  uint8_t scope;
  uint32_t flags; /* NQ_NET_IFA_* */
  uint32_t have;  /* NQ_NET_ADDR_* */
  uint8_t local[16];
  uint8_t address[16];
  uint8_t broadcast[16];
  uint8_t anycast[16];
  char label[NQ_NET_NAME_SIZE];
  uint32_t validLifetime, preferredLifetime; /* seconds, or NQ_NET_LIFETIME_INFINITE */
  uint32_t metric;
} NQNetAddress;

typedef struct NQNetNexthop {
  int gatewayFamily; /* NQ_NET_UNSPEC without one */
  uint8_t gateway[16];
  unsigned index;
  uint8_t hops; /* the weight less one */
  unsigned flags;
} NQNetNexthop;

/* What a NQNetRoute has: its `have` bits */
enum {
  NQ_NET_ROUTE_DST = 1 << 0,
  NQ_NET_ROUTE_SRC = 1 << 1,
  NQ_NET_ROUTE_GATEWAY = 1 << 2,
  NQ_NET_ROUTE_VIA = 1 << 3, /* a gateway of another family */
  NQ_NET_ROUTE_PREFSRC = 1 << 4,
  NQ_NET_ROUTE_OIF = 1 << 5,
  NQ_NET_ROUTE_IIF = 1 << 6,
  NQ_NET_ROUTE_PRIORITY = 1 << 7,
  NQ_NET_ROUTE_MARK = 1 << 8,
  NQ_NET_ROUTE_UID = 1 << 9,
  NQ_NET_ROUTE_PREF = 1 << 10,
  NQ_NET_ROUTE_CACHEINFO = 1 << 11,
  NQ_NET_ROUTE_EXPIRES = 1 << 12,
  NQ_NET_ROUTE_METRICS = 1 << 13,
  NQ_NET_ROUTE_FLOW = 1 << 14,
  NQ_NET_ROUTE_NHID = 1 << 15,
  NQ_NET_ROUTE_TABLE = 1 << 16, /* for a change: `table` was given */
};

/* Route metrics: Linux's RTAX_* numbers */
enum {
  NQ_NET_RTAX_LOCK = 1,
  NQ_NET_RTAX_MTU = 2,
  NQ_NET_RTAX_WINDOW = 3,
  NQ_NET_RTAX_RTT = 4,
  NQ_NET_RTAX_RTTVAR = 5,
  NQ_NET_RTAX_SSTHRESH = 6,
  NQ_NET_RTAX_CWND = 7,
  NQ_NET_RTAX_ADVMSS = 8,
  NQ_NET_RTAX_REORDERING = 9,
  NQ_NET_RTAX_HOPLIMIT = 10,
  NQ_NET_RTAX_INITCWND = 11,
  NQ_NET_RTAX_FEATURES = 12,
  NQ_NET_RTAX_RTO_MIN = 13,
  NQ_NET_RTAX_INITRWND = 14,
  NQ_NET_RTAX_QUICKACK = 15,
  NQ_NET_RTAX_CC_ALGO = 16,
  NQ_NET_RTAX_FASTOPEN_NO_COOKIE = 17,
  NQ_NET_RTAX_MAX = 17,
};

typedef struct NQNetRoute {
  int family;
  uint8_t dstLength, srcLength;
  uint8_t tos;
  uint8_t protocol; /* RTPROT_* */
  uint8_t scope;    /* RT_SCOPE_* */
  uint8_t type;     /* RTN_* */
  uint32_t table;   /* RT_TABLE_* or a number */
  unsigned flags;   /* RTNH_F_*, RTM_F_* and RTCF_* */
  uint32_t have;    /* NQ_NET_ROUTE_* */
  uint8_t dst[16], src[16], gateway[16], prefsrc[16];
  int viaFamily;
  uint8_t via[16];
  unsigned oif, iif;
  uint32_t priority, mark, uid, flow, nhid;
  uint8_t pref; /* ICMPV6_ROUTER_PREF_*: 0 medium, 1 high, 3 low */
  /* the cache information, in seconds where it is a time */
  int32_t expires;
  uint32_t error, users, used, age, ipid, ts, tsage;
  /* RTAX_* values: metrics[i] counts where bit i of metricsHave is set */
  uint32_t metrics[NQ_NET_RTAX_MAX + 1];
  uint32_t metricsHave, metricsLock;
  char congestion[16]; /* RTAX_CC_ALGO */
  NQNetNexthop* nexthops; /* a multipath route; NQNetFreeRoutes frees them */
  size_t nexthopCount;
} NQNetRoute;

/* What a NQNetNeighbor has: its `have` bits */
enum {
  NQ_NET_NEIGH_DST = 1 << 0,
  NQ_NET_NEIGH_LLADDR = 1 << 1,
  NQ_NET_NEIGH_CACHEINFO = 1 << 2,
  NQ_NET_NEIGH_PROBES = 1 << 3,
  NQ_NET_NEIGH_PROTOCOL = 1 << 4,
  NQ_NET_NEIGH_EXT_FLAGS = 1 << 5,
};

typedef struct NQNetNeighbor {
  int family;
  unsigned index;
  uint16_t state; /* NUD_* */
  uint8_t flags;  /* NTF_* */
  uint32_t have;  /* NQ_NET_NEIGH_* */
  uint8_t dst[16];
  uint8_t lladdr[NQ_NET_HWADDR_SIZE];
  size_t lladdrLength;
  uint8_t protocol;
  uint32_t extFlags;
  /* seconds since it was used, confirmed and updated */
  uint32_t refcnt, used, confirmed, updated, probes;
} NQNetNeighbor;

/* What a change does */
typedef enum NQNetChange {
  NQ_NET_ADD,     /* create; fails if it exists */
  NQ_NET_CHANGE,  /* change; fails if it does not exist */
  NQ_NET_REPLACE, /* create or change */
  NQ_NET_PREPEND, /* create, before others alike (routes) */
  NQ_NET_APPEND,  /* create, after others alike (routes) */
  NQ_NET_TEST,    /* fails if it exists, creates nothing (routes) */
  NQ_NET_DELETE,
} NQNetChange;

/* A link to change: `index` names it (0 with NQ_NET_ADD) */
enum {
  NQ_NET_SET_NAME = 1 << 0,
  NQ_NET_SET_MTU = 1 << 1,
  NQ_NET_SET_TXQLEN = 1 << 2,
  NQ_NET_SET_ADDRESS = 1 << 3,
  NQ_NET_SET_BROADCAST = 1 << 4,
  NQ_NET_SET_MASTER = 1 << 5, /* master 0 detaches it */
  NQ_NET_SET_ALIAS = 1 << 6,
  NQ_NET_SET_GROUP = 1 << 7,
  NQ_NET_SET_LINKMODE = 1 << 8,
  NQ_NET_SET_OPERSTATE = 1 << 9,
  NQ_NET_SET_KIND = 1 << 10, /* the type of a link to create */
  NQ_NET_SET_LINK = 1 << 11, /* the link it sits on */
  NQ_NET_SET_PEER = 1 << 12, /* veth: the name of the other end */
  NQ_NET_SET_CARRIER = 1 << 13,
  NQ_NET_SET_PROTODOWN = 1 << 14,
};

typedef struct NQNetLinkRequest {
  unsigned index;
  uint32_t set;       /* NQ_NET_SET_* */
  unsigned flags;     /* NQ_NET_IFF_* to set ... */
  unsigned flagsMask; /* ... where these bits are */
  char name[NQ_NET_NAME_SIZE];
  unsigned mtu;
  uint32_t txqlen;
  uint8_t address[NQ_NET_HWADDR_SIZE];
  size_t addressLength;
  uint8_t broadcast[NQ_NET_HWADDR_SIZE];
  size_t broadcastLength;
  unsigned master;
  char alias[256];
  uint32_t group;
  uint8_t linkMode;
  uint8_t operState;
  uint8_t carrier;
  uint8_t protoDown;
  char kind[32];
  unsigned link;
  char peer[NQ_NET_NAME_SIZE];
} NQNetLinkRequest;

/* What NQNetRouteGet looks up */
enum {
  NQ_NET_QUERY_SRC = 1 << 0,
  NQ_NET_QUERY_IIF = 1 << 1,
  NQ_NET_QUERY_OIF = 1 << 2,
  NQ_NET_QUERY_MARK = 1 << 3,
  NQ_NET_QUERY_UID = 1 << 4,
  NQ_NET_QUERY_FIBMATCH = 1 << 5, /* the route itself, not a cache entry */
};

typedef struct NQNetRouteQuery {
  int family;
  uint32_t have; /* NQ_NET_QUERY_* */
  uint8_t dst[16];
  uint8_t dstLength;
  uint8_t src[16];
  uint8_t srcLength;
  uint8_t tos;
  unsigned iif, oif;
  uint32_t mark, uid;
} NQNetRouteQuery;

/* Each list is allocated with NQMalloc; free it with the matching free
 * function. `family` picks NQ_NET_INET or NQ_NET_INET6, or both with
 * NQ_NET_UNSPEC. */
NQ_EXPORT int NQNetGetLinks(NQNetLink** links, size_t* count);
NQ_EXPORT int NQNetGetAddresses(int family, NQNetAddress** addresses, size_t* count);
NQ_EXPORT int NQNetGetRoutes(int family, NQNetRoute** routes, size_t* count);
NQ_EXPORT int NQNetGetNeighbors(int family, NQNetNeighbor** neighbors, size_t* count);
NQ_EXPORT void NQNetFreeLinks(NQNetLink* links, size_t count);
NQ_EXPORT void NQNetFreeAddresses(NQNetAddress* addresses, size_t count);
NQ_EXPORT void NQNetFreeRoutes(NQNetRoute* routes, size_t count);
NQ_EXPORT void NQNetFreeNeighbors(NQNetNeighbor* neighbors, size_t count);

/* The route a packet to query->dst takes; free it with
 * NQNetFreeRoutes(route, 1) */
NQ_EXPORT int NQNetRouteGet(const NQNetRouteQuery* query, NQNetRoute** route);

/* Changes; these need root (CAP_NET_ADMIN) or Administrator */
NQ_EXPORT int NQNetLinkChange(NQNetChange change, const NQNetLinkRequest* request);
NQ_EXPORT int NQNetAddressChange(NQNetChange change, const NQNetAddress* address);
NQ_EXPORT int NQNetRouteChange(NQNetChange change, const NQNetRoute* route);
NQ_EXPORT int NQNetNeighborChange(NQNetChange change, const NQNetNeighbor* neighbor);

/* What the system said about the last change or lookup that failed on
 * this thread, as Linux's extended acknowledgements say it ("ipv4:
 * Address already assigned"), or "" when it said nothing more than the
 * error code */
NQ_EXPORT const char* NQNetGetErrorMessage(void);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_NET_NETCONFIG_H */
