/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/net/GetAddrInfo.h"

#include <libnetq/net/SockAddr.h>
#include <libnetq/string/String.h>
#include <libnetq/string/StringUtil.h>
#include <libnetq/Malloc.h>
#include <libnetq/Endian.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/Assert.h>

#if defined(NQ_OS_UNIX)
# include <netdb.h>
#elif defined(NQ_OS_WINDOWS)
# include <ws2tcpip.h>
# include <libnetq/backend/win32/Socket.h>
#endif

void NQFreeAddrInfo(NQAddrInfo* list)
{
  while (list != NULL) {
    NQAddrInfo* next = list->next;
    NQCStrFree(list->canonname);
    NQFree(list);
    list = next;
  }
}

const char* NQGaiStrError(int code)
{
  switch (code) {
  case 0:
    return "Success";
  case NQ_EAI_BADFLAGS:
    return "Bad value for ai_flags";
  case NQ_EAI_NONAME:
    return "Name or service not known";
  case NQ_EAI_AGAIN:
    return "Temporary failure in name resolution";
  case NQ_EAI_FAIL:
    return "Non-recoverable failure in name resolution";
  case NQ_EAI_NODATA:
    return "No address associated with hostname";
  case NQ_EAI_FAMILY:
    return "ai_family not supported";
  case NQ_EAI_SOCKTYPE:
    return "ai_socktype not supported";
  case NQ_EAI_SERVICE:
    return "Servname not supported for ai_socktype";
  case NQ_EAI_ADDRFAMILY:
    return "Address family for hostname not supported";
  case NQ_EAI_MEMORY:
    return "Memory allocation failure";
  case NQ_EAI_SYSTEM:
    return "System error";
  case NQ_EAI_OVERFLOW:
    return "Argument buffer overflow";
  }
  return "Unknown error";
}

#if defined(NQ_OS_UNIX) || defined(NQ_OS_WINDOWS)

typedef struct FlagPair {
  int nq;
  int platform;
} FlagPair;

static const FlagPair s_flags[] = {
  { NQ_AI_PASSIVE, AI_PASSIVE },
  { NQ_AI_CANONNAME, AI_CANONNAME },
  { NQ_AI_NUMERICHOST, AI_NUMERICHOST },
#ifdef AI_V4MAPPED
  { NQ_AI_V4MAPPED, AI_V4MAPPED },
#endif
#ifdef AI_ALL
  { NQ_AI_ALL, AI_ALL },
#endif
#ifdef AI_ADDRCONFIG
  { NQ_AI_ADDRCONFIG, AI_ADDRCONFIG },
#endif
#ifdef AI_NUMERICSERV
  { NQ_AI_NUMERICSERV, AI_NUMERICSERV },
#endif
};

/* A table rather than a switch: some platforms give two EAI_* names one value. */
static const FlagPair s_errors[] = {
  { NQ_EAI_BADFLAGS, EAI_BADFLAGS },
  { NQ_EAI_NONAME, EAI_NONAME },
  { NQ_EAI_AGAIN, EAI_AGAIN },
  { NQ_EAI_FAIL, EAI_FAIL },
  { NQ_EAI_FAMILY, EAI_FAMILY },
  { NQ_EAI_SOCKTYPE, EAI_SOCKTYPE },
  { NQ_EAI_SERVICE, EAI_SERVICE },
  { NQ_EAI_MEMORY, EAI_MEMORY },
#ifdef EAI_NODATA
  { NQ_EAI_NODATA, EAI_NODATA },
#endif
#ifdef EAI_ADDRFAMILY
  { NQ_EAI_ADDRFAMILY, EAI_ADDRFAMILY },
#endif
#ifdef EAI_SYSTEM
  { NQ_EAI_SYSTEM, EAI_SYSTEM },
#endif
#ifdef EAI_OVERFLOW
  { NQ_EAI_OVERFLOW, EAI_OVERFLOW },
#endif
};

static bool toPlatformFlags(int flags, int* result)
{
  int platform = 0;
  for (size_t i = 0; i < NQ_ARRAY_LENGTH(s_flags); i++) {
    if (flags & s_flags[i].nq) {
      platform |= s_flags[i].platform;
      flags &= ~s_flags[i].nq;
    }
  }
  *result = platform;
  return flags == 0;
}

static int fromPlatformFlags(int platform)
{
  int flags = 0;
  for (size_t i = 0; i < NQ_ARRAY_LENGTH(s_flags); i++) {
    if (platform & s_flags[i].platform)
      flags |= s_flags[i].nq;
  }
  return flags;
}

static int fromPlatformError(int code)
{
  for (size_t i = 0; i < NQ_ARRAY_LENGTH(s_errors); i++) {
    if (code == s_errors[i].platform)
      return s_errors[i].nq;
  }
  return NQ_EAI_FAIL;
}

/* NQ_AF_* matches AF_* on Unix only; Windows numbers AF_INET6 differently. */
static bool toPlatformFamily(int family, int* result)
{
  switch (family) {
  case NQ_AF_UNSPEC:
    *result = AF_UNSPEC;
    return true;
  case NQ_AF_INET:
    *result = AF_INET;
    return true;
  case NQ_AF_INET6:
    *result = AF_INET6;
    return true;
  }
  return false;
}

/* Appends `ai` to the list unless its address family is neither IPv4 nor IPv6. */
static bool appendResult(const struct addrinfo* ai, NQAddrInfo*** tail)
{
  NQEndPoint address;
  if (ai->ai_family == AF_INET && ai->ai_addrlen >= sizeof(NQSockAddrIn)) {
    const NQSockAddrIn* in4 = (const NQSockAddrIn*)ai->ai_addr;
    NQEndPoint_init4(&address, (const uint8_t*)&in4->sin_addr.s_addr, NQNtohs(in4->sin_port));
  } else if (ai->ai_family == AF_INET6 && ai->ai_addrlen >= sizeof(NQSockAddrIn6)) {
    const NQSockAddrIn6* in6 = (const NQSockAddrIn6*)ai->ai_addr;
    NQEndPoint_init6(&address, (const uint8_t*)&in6->sin6_addr.s6_addr[0], NQNtohs(in6->sin6_port));
  } else {
    return true;
  }

  NQAddrInfo* item = (NQAddrInfo*)NQZeroMalloc(sizeof(*item));
  if (item == NULL)
    return false;
  item->flags = fromPlatformFlags(ai->ai_flags);
  item->family = address.family;
  item->socktype = ai->ai_socktype;
  item->protocol = ai->ai_protocol;
  item->address = address;
  if (ai->ai_canonname != NULL) {
    item->canonname = NQCStrDuplicate(ai->ai_canonname);
    if (item->canonname == NULL) {
      NQFree(item);
      return false;
    }
  }

  **tail = item;
  *tail = &item->next;
  return true;
}

int NQGetAddrInfo(const char* node, const char* service, const NQAddrInfo* hints, NQAddrInfo** result)
{
#ifdef NQ_OS_WINDOWS
  NQWinSocketInitialize();
#endif

  struct addrinfo platformHints;
  if (hints != NULL) {
    NQMemset(&platformHints, 0, sizeof(platformHints));
    if (!toPlatformFlags(hints->flags, &platformHints.ai_flags))
      return NQ_EAI_BADFLAGS;
    if (!toPlatformFamily(hints->family, &platformHints.ai_family))
      return NQ_EAI_FAMILY;
    /* NQ_SOCK_* and NQ_IPPROTO_* share their numbers with every platform. */
    platformHints.ai_socktype = hints->socktype;
    platformHints.ai_protocol = hints->protocol;
  }

  struct addrinfo* list = NULL;
  int ret = getaddrinfo(node, service, hints != NULL ? &platformHints : NULL, &list);
  if (ret != 0)
    return fromPlatformError(ret);

  NQAddrInfo* head = NULL;
  NQAddrInfo** tail = &head;
  for (const struct addrinfo* ai = list; ai != NULL; ai = ai->ai_next) {
    if (!appendResult(ai, &tail)) {
      freeaddrinfo(list);
      NQFreeAddrInfo(head);
      return NQ_EAI_MEMORY;
    }
  }
  freeaddrinfo(list);

  /* Only non-IP families came back, which is as good as no address at all. */
  if (head == NULL)
    return NQ_EAI_NONAME;

  *result = head;
  return 0;
}

int NQGetNameInfo(const NQEndPoint* address, char* host, size_t hostLen, int flags)
{
#ifdef NQ_OS_WINDOWS
  NQWinSocketInitialize();
#endif

  NQSockAddrStorage storage;
  socklen_t len;
  NQMemset(&storage, 0, sizeof(storage));
  if (address->family == NQ_AF_INET) {
    NQSockAddrIn* in4 = (NQSockAddrIn*)&storage;
    in4->sin_family = AF_INET;
    NQMemcpy(&in4->sin_addr.s_addr, address->ip4ep.address.data, 4);
    len = sizeof(*in4);
  } else if (address->family == NQ_AF_INET6) {
    NQSockAddrIn6* in6 = (NQSockAddrIn6*)&storage;
    in6->sin6_family = AF_INET6;
    NQMemcpy(&in6->sin6_addr.s6_addr[0], address->ip6ep.address.data, 16);
    len = sizeof(*in6);
  } else {
    return NQ_EAI_FAMILY;
  }

  int platform = 0;
  if (flags & NQ_NI_NUMERICHOST)
    platform |= NI_NUMERICHOST;
  if (flags & NQ_NI_NAMEREQD)
    platform |= NI_NAMEREQD;
  int ret = getnameinfo((const struct sockaddr*)&storage, len, host, (socklen_t)hostLen, NULL, 0, platform);
  return ret != 0 ? fromPlatformError(ret) : 0;
}

#else

int NQGetNameInfo(const NQEndPoint* address, char* host, size_t hostLen, int flags)
{
  NQ_UNUSED_PARAM(address);
  NQ_UNUSED_PARAM(host);
  NQ_UNUSED_PARAM(hostLen);
  NQ_UNUSED_PARAM(flags);

  NQSetLastError(NQ_ENOTSUPP);
  return NQ_EAI_SYSTEM;
}

int NQGetAddrInfo(const char* node, const char* service, const NQAddrInfo* hints, NQAddrInfo** result)
{
  NQ_UNUSED_PARAM(node);
  NQ_UNUSED_PARAM(service);
  NQ_UNUSED_PARAM(hints);
  NQ_UNUSED_PARAM(result);

  NQSetLastError(NQ_ENOTSUPP);
  return NQ_EAI_SYSTEM;
}

#endif
