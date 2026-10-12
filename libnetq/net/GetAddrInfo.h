/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_NET_GETADDRINFO_H
#define _LIBNETQ_NET_GETADDRINFO_H

#include <libnetq/net/Netinet.h>
#include <libnetq/net/EndPoint.h>
#include <libnetq/net/Socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/* NQAddrInfo.flags, as AI_* of getaddrinfo(3). */
enum {
  NQ_AI_PASSIVE = 0x0001,
  NQ_AI_CANONNAME = 0x0002,
  NQ_AI_NUMERICHOST = 0x0004,
  NQ_AI_V4MAPPED = 0x0008,
  NQ_AI_ALL = 0x0010,
  NQ_AI_ADDRCONFIG = 0x0020,
  NQ_AI_NUMERICSERV = 0x0400,
};

/* Return codes of NQGetAddrInfo, with the values glibc gives EAI_* on every platform. */
enum {
  NQ_EAI_BADFLAGS = -1,
  NQ_EAI_NONAME = -2,
  NQ_EAI_AGAIN = -3,
  NQ_EAI_FAIL = -4,
  NQ_EAI_NODATA = -5,
  NQ_EAI_FAMILY = -6,
  NQ_EAI_SOCKTYPE = -7,
  NQ_EAI_SERVICE = -8,
  NQ_EAI_ADDRFAMILY = -9,
  NQ_EAI_MEMORY = -10,
  NQ_EAI_SYSTEM = -11, /* see NQGetLastError() */
  NQ_EAI_OVERFLOW = -12,
};

typedef struct NQAddrInfo NQAddrInfo;

/* struct addrinfo, with the socket address as an NQEndPoint. */
struct NQAddrInfo {
  int flags;      /* NQ_AI_* */
  int family;     /* NQ_AF_INET or NQ_AF_INET6 in results */
  int socktype;   /* NQ_SOCK_*, 0 for any */
  int protocol;   /* NQ_IPPROTO_*, 0 for any */
  NQEndPoint address;
  char* canonname;
  NQAddrInfo* next;
};

/*
 * Resolves `node` and/or `service` like getaddrinfo(3): only flags, family,
 * socktype and protocol of `hints` are read, and NULL hints behave as on Linux.
 * Returns 0 and a list for NQFreeAddrInfo in *result, or an NQ_EAI_* code.
 * Blocks while the name is looked up.
 */
NQ_EXPORT int NQGetAddrInfo(const char* node, const char* service, const NQAddrInfo* hints, NQAddrInfo** result);
NQ_EXPORT void NQFreeAddrInfo(NQAddrInfo* list);

/* Message for an NQ_EAI_* code, as gai_strerror(3) on Linux. */
NQ_EXPORT const char* NQGaiStrError(int code);

/* NQGetNameInfo flags, as NI_* of getnameinfo(3). */
enum {
  NQ_NI_NUMERICHOST = 0x0001,
  NQ_NI_NAMEREQD = 0x0008,
};

/*
 * The host name of `address` like getnameinfo(3) (the port is left out):
 * the name, or the numeric address unless NQ_NI_NAMEREQD asks for a name.
 * Returns 0, or an NQ_EAI_* code. Blocks while the name is looked up.
 */
NQ_EXPORT int NQGetNameInfo(const NQEndPoint* address, char* host, size_t hostLen, int flags);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_NET_GETADDRINFO_H */
