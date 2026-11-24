/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_NET_ICMP_H
#define _LIBNETQ_NET_ICMP_H

#include <libnetq/Basic.h>
#include <libnetq/net/EndPoint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ICMP echo (ping) without privileges where the system allows it: a "ping
 * socket" (SOCK_DGRAM, IPPROTO_ICMP) on Linux, macOS and Android, else a
 * raw socket (root or CAP_NET_RAW); IcmpSendEcho2 / Icmp6SendEcho2 on
 * Windows, one echo request at a time. Non-blocking: wait on
 * NQIcmpSocket_handle (a socket to read from, or on Windows an event)
 * and call NQIcmpSocket_receive until it returns 0.
 */
typedef struct NQIcmpSocket NQIcmpSocket;

enum {
  NQ_ICMP_ECHO_REPLY = 1, /* the reply to an echo request */
  NQ_ICMP_ERROR = 2,      /* an error an echo request caused, from `from` */
};

typedef struct NQIcmpReply {
  int kind;          /* NQ_ICMP_* */
  int type, code;    /* as received: ICMP numbers for IPv4, ICMPv6 for IPv6 */
  uint16_t id, seq;  /* of the echo request (for an error, the one it quotes) */
  int ttl;           /* TTL or hop limit of the reply, -1 when unknown */
  size_t length;     /* bytes of ICMP received: header and data */
  uint32_t info;     /* the MTU of "fragmentation needed" or "packet too big" */
  NQEndPoint from;
} NQIcmpReply;

NQ_EXPORT int NQIcmpSocket_open(int family, NQIcmpSocket** result);
NQ_EXPORT void NQIcmpSocket_close(NQIcmpSocket*);
NQ_EXPORT intptr_t NQIcmpSocket_handle(NQIcmpSocket*);
NQ_EXPORT bool NQIcmpSocket_isRaw(NQIcmpSocket*);
NQ_EXPORT bool NQIcmpSocket_ownsReplies(NQIcmpSocket*);
NQ_EXPORT int NQIcmpSocket_setTtl(NQIcmpSocket*, int ttl);
NQ_EXPORT int NQIcmpSocket_connect(NQIcmpSocket*, const NQEndPoint* to);
NQ_EXPORT int NQIcmpSocket_sendEcho(NQIcmpSocket*, uint16_t id, uint16_t seq, const void* data, size_t len);
NQ_EXPORT int NQIcmpSocket_receive(NQIcmpSocket*, NQIcmpReply* reply);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_NET_ICMP_H */
