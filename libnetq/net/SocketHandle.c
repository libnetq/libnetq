/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/net/SocketHandle.h"

#include <libnetq/net/PlatformSocket.h>
#include <libnetq/string/String.h> // for memset
#include <libnetq/ErrorCode.h>
#include <libnetq/Assert.h>

#ifdef NQ_OS_UNIX
#include <sys/ioctl.h> // for ioctl
#include <sys/select.h> // for FD_SETSIZE
#include <fcntl.h> // for fcntl
#endif

#ifdef NQ_OS_WINDOWS

#include <libnetq/sync/Once.h>

NQ_STATIC_ASSERT(sizeof(NQSocketHandle) == sizeof(NQPlatformSocket), "Socket size did not match in Windows");

static WSADATA s_wsadata;
static NQOnce s_once = NQ_ONCE_INIT;

static void WSAStartupInit(void)
{
  int status = WSAStartup(MAKEWORD(2, 2), &s_wsadata);
  NQ_ASSERT_UNUSED(status, !status);
}

static inline void socketInit(void)
{
  NQOnce_call(&s_once, &WSAStartupInit);
}

static int normalizeSockFamily(int family)
{
  switch (family) {
  case NQ_AF_INET:
    return AF_INET;
  case NQ_AF_INET6:
    return AF_INET6;
  }
  NQ_ASSERT_NOT_REACHED();
  return family;
}

static int normalizeSockType(int type)
{
  switch (type) {
  case NQ_SOCK_STREAM:
    return SOCK_STREAM;
  case NQ_SOCK_DGRAM:
    return SOCK_DGRAM;
  }
  NQ_ASSERT_NOT_REACHED();
  return type;
}

static int normalizeSockProtocol(int protocol)
{
  switch (protocol) {
  case NQ_IPPROTO_IP:
    return IPPROTO_IP;
  case NQ_IPPROTO_TCP:
    return IPPROTO_TCP;
  case NQ_IPPROTO_UDP:
    return IPPROTO_UDP;
  }
  NQ_ASSERT_NOT_REACHED();
  return protocol;
}

static int normalizeShutdownHow(int how)
{
  switch (how) {
  case NQ_SHUT_RD:
    return SD_RECEIVE;
  case NQ_SHUT_WR:
    return SD_SEND;
  case NQ_SHUT_RDWR:
    return SD_BOTH;
  }
  NQ_ASSERT_NOT_REACHED();
  return how;
}

static int normalizeOptLevel(int opt)
{
  switch (opt) {
  case NQ_SOL_SOCKET:
    return SOL_SOCKET;
  case NQ_IPPROTO_TCP:
    return IPPROTO_TCP;
  }
  NQ_ASSERT_NOT_REACHED();
  return opt;
}

static int normalizeOptName(int opt)
{
  switch (opt) {
  case NQ_SO_ERROR:
    return SO_ERROR;
  case NQ_SO_REUSEADDR:
    return SO_REUSEADDR;
  case NQ_SO_BROADCAST:
    return SO_BROADCAST;
  case NQ_TCP_NODELAY:
    return TCP_NODELAY;
  }
  NQ_ASSERT_NOT_REACHED();
  return opt;
}
#else

static inline void socketInit(void)
{
}

static inline int normalizeSockFamily(int family)
{
  return family;
}

static inline int normalizeSockType(int type)
{
  return type;
}

static int normalizeSockProtocol(int protocol)
{
  return protocol;
}

static inline int  normalizeShutdownHow(int how)
{
  return how;
}

static inline int normalizeOptLevel(int opt)
{
  return opt;
}

static inline int normalizeOptName(int opt)
{
  return opt;
}

#endif

static void NQIPv4EndPoint_initWithInet4(NQIPv4EndPoint* thiz, const struct sockaddr_in* addr)
{
  memcpy(thiz->address.data, &addr->sin_addr.s_addr, 4);
  thiz->port = htons(addr->sin_port);
}

static int NQIPv4EndPoint_toInet4(const NQIPv4EndPoint* ep, struct sockaddr_in* result)
{
  memset(result, 0, sizeof(*result));
  result->sin_family = AF_INET;
  memcpy(&result->sin_addr.s_addr, ep->address.data, 4);
  result->sin_port = htons(ep->port);
  return sizeof(*result);
}

static void NQIPv6EndPoint_initWithInet6(NQIPv6EndPoint* thiz, const struct sockaddr_in6* addr)
{
  memcpy(&thiz->address.data[0], &addr->sin6_addr.s6_addr[0], sizeof(addr->sin6_addr.s6_addr));
  thiz->port = htons(addr->sin6_port);
}

static int NQIPv6EndPoint_toInet6(const NQIPv6EndPoint* ep, struct sockaddr_in6* result)
{
  memset(result, 0, sizeof(*result));
  result->sin6_family = AF_INET6;
  memcpy(&result->sin6_addr.s6_addr[0], &ep->address.data[0], sizeof(ep->address.data));
  result->sin6_port = htons(ep->port);
  return sizeof(*result);
}

static bool NQEndPoint_initWithInet(NQEndPoint* thiz, const struct sockaddr* addr, int len)
{
  if (addr->sa_family == AF_INET) {
    if (len < sizeof(NQSockAddrIn))
      return false;
    thiz->family = NQ_AF_INET;
    NQIPv4EndPoint_initWithInet4(&thiz->ip4ep, (const struct sockaddr_in*)addr);
    return true;
  }

  if (addr->sa_family == AF_INET6) {
    if (len < sizeof(NQSockAddrIn6))
      return false;
    thiz->family = NQ_AF_INET6;
    NQIPv6EndPoint_initWithInet6(&thiz->ip6ep, (const struct sockaddr_in6*)addr);
    return true;
  }

  return false;
}

static int NQEndPoint_toInet(const NQEndPoint* thiz, struct sockaddr* addr, int len)
{
  if (thiz->family == NQ_AF_INET) {
    if (len < sizeof(NQSockAddrIn))
      return 0;
    return NQIPv4EndPoint_toInet4(&thiz->ip4ep, (struct sockaddr_in*)addr);
  }

  if (thiz->family == NQ_AF_INET6) {
    if (len < sizeof(NQSockAddrIn6))
      return 0;
    return NQIPv6EndPoint_toInet6(&thiz->ip6ep, (struct sockaddr_in6*)addr);
  }

  return 0;
}

int NQSocketOpen(int family, int type, int protocol, NQSocketHandle* result)
{
  socketInit();
  return NQPlatformSocketOpen(normalizeSockFamily(family), normalizeSockType(type), normalizeSockProtocol(protocol), result);
}

void NQSocketClose(NQSocketHandle handle)
{
  NQPlatformSocketClose(handle);
}

int NQSocketSend(NQSocketHandle handle, const void* buf, size_t len, int flags)
{
  return NQPlatformSocketSend(handle, buf, len, flags);
}

int NQSocketRecv(NQSocketHandle handle, void* buf, size_t len, int flags)
{
  return NQPlatformSocketRecv(handle, buf, len, flags);
}

int NQSocketShutdown(NQSocketHandle handle, int how)
{
  return NQPlatformSocketShutdown(handle, normalizeShutdownHow(how));
}

int NQSocketGetOpt(NQSocketHandle handle, int level, int optname, void* optval, int* optlen)
{
  return NQPlatformSocketGetOpt(handle, normalizeOptLevel(level), normalizeOptName(optname), optval, optlen);
}

int NQSocketSetOpt(NQSocketHandle handle, int level, int optname, const void* optval, int optlen)
{
  return NQPlatformSocketSetOpt(handle, normalizeOptLevel(level), normalizeOptName(optname), optval, optlen);
}

int NQSocketSetNonBlocking(NQSocketHandle handle, bool blocking)
{
#if defined(NQ_OS_WINDOWS)
  unsigned long opt = blocking ? 1 : 0;
  int ret = ioctlsocket(handle, FIONBIO, &opt);
  if (ret == SOCKET_ERROR)
    return -WSAGetLastError();
  return ret;

#elif defined(NQ_OS_UNIX)
#ifdef O_NONBLOCK
  /* FIXME: O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. */
  int flags = fcntl(handle, F_GETFL, 0);
  if (flags == -1)
    flags = 0;

  if (blocking)
    flags |= O_NONBLOCK;
  else
    flags &= ~O_NONBLOCK;

  int ret = fcntl(handle, F_SETFL, flags);
  if (ret == -1)
    return -errno;
  return ret;

#else
  int opt = blocking ? 1 : 0;
  int ret = ioctl(handle, FIONBIO, &opt);
  if (ret == -1)
    return -errno;
  return ret;

#endif
#else
  return -NQ_ENOTSUPP;

#endif
}

int NQSocketConnect(NQSocketHandle handle, const NQEndPoint* ep)
{
  if (ep->family == NQ_AF_INET)
    return NQSocketConnect4(handle, &ep->ip4ep);

  if (ep->family == NQ_AF_INET6)
    return NQSocketConnect6(handle, &ep->ip6ep);

  NQ_ASSERT_NOT_REACHED();
  return -NQ_ENOTSUPP;
}

int NQSocketConnect4(NQSocketHandle handle, const NQIPv4EndPoint* ep)
{
  NQ_ASSERT(ep);
  struct sockaddr_in addr;
  int size = NQIPv4EndPoint_toInet4(ep, &addr);
  return NQPlatformSocketConnect(handle, (struct sockaddr*)&addr, size);
}

int NQSocketConnect6(NQSocketHandle handle, const NQIPv6EndPoint* ep)
{
  NQ_ASSERT(ep);
  struct sockaddr_in6 addr;
  int size = NQIPv6EndPoint_toInet6(ep, &addr);
  return NQPlatformSocketConnect(handle, (struct sockaddr*)&addr, size);
}

int NQSocketAccept(NQSocketHandle handle, NQEndPoint* ep, NQSocketHandle* result)
{
  NQUnionSockAddr addr;
  int len = sizeof(addr);
  NQSocketHandle acceptHandle;
  int ret = NQPlatformSocketAccept(handle, &addr.sa, &len, &acceptHandle);
  if (ret != 0)
    return ret;

  if (ep != NULL && !NQEndPoint_initWithInet(ep, &addr.sa, len)) {
    NQ_ASSERT_NOT_REACHED();
    NQPlatformSocketClose(acceptHandle);
    return -NQ_EINVAL;
  }

  *result = acceptHandle;
  return 0;
}

int NQSocketBind(NQSocketHandle handle, const NQEndPoint* ep)
{
  NQUnionSockAddr addr;
  int size = NQEndPoint_toInet(ep, &addr.sa, sizeof(addr));
  return NQPlatformSocketBind(handle, &addr.sa, size);
}

int NQSocketBind4(NQSocketHandle handle, const NQIPv4EndPoint* ep)
{
  NQUnionSockAddr addr;
  int size = NQIPv4EndPoint_toInet4(ep, &addr.in4);
  return NQPlatformSocketBind(handle, &addr.sa, size);
}

int NQSocketBind6(NQSocketHandle handle, const NQIPv6EndPoint* ep)
{
  NQUnionSockAddr addr;
  int size = NQIPv6EndPoint_toInet6(ep, &addr.in6);
  return NQPlatformSocketBind(handle, &addr.sa, size);
}

int NQSocketListen(NQSocketHandle handle, int backlog)
{
  return NQPlatformSocketListen(handle, backlog);
}

int NQSocketSendTo(NQSocketHandle handle, const uint8_t* buf, size_t len, int flags, const NQEndPoint* ep)
{
  if (ep->family == NQ_AF_INET)
    return NQSocketSendTo4(handle, buf, len, flags, &ep->ip4ep);

  if (ep->family == NQ_AF_INET6)
    return NQSocketSendTo6(handle, buf, len, flags, &ep->ip6ep);

  NQ_ASSERT_NOT_REACHED();
  return -NQ_ENOTSUPP;
}

int NQSocketSendTo4(NQSocketHandle handle, const uint8_t* buf, size_t len, int flags, const NQIPv4EndPoint* ep)
{
  NQSockAddrIn addr;
  int size = NQIPv4EndPoint_toInet4(ep, &addr);
  return NQPlatformSocketSendto(handle, buf, len, flags, (struct sockaddr*)&addr, size);
}

int NQSocketSendTo6(NQSocketHandle handle, const uint8_t* buf, size_t len, int flags, const NQIPv6EndPoint* ep)
{
  NQSockAddrIn6 addr;
  int size = NQIPv6EndPoint_toInet6(ep, &addr);
  return NQPlatformSocketSendto(handle, buf, len, flags, (struct sockaddr*)&addr, size);
}

bool NQSocketIsSelectable(NQSocketHandle handle)
{
#if defined(NQ_OS_UNIX) || defined(NQ_OS_WINDOWS)
  return handle < FD_SETSIZE;
#else
  return false;
#endif
}

int NQSocketPair(int family, int type, int protocol, NQSocketHandle sock[2])
{
  socketInit();

#if defined(NQ_OS_UNIX)
  return socketpair(AF_LOCAL, type, protocol, sock);

#elif defined(NQ_OS_WINDOWS)
  struct sockaddr_in address;
  NQSocketHandle listener;
  int size = sizeof(address);
  int ret;

  NQ_ASSERT(family == NQ_AF_INET || family == NQ_AF_INET6);

  ret = NQSocketOpen(family, type, protocol, &listener);
  if (ret != 0)
    return ret;

  // ignore errors coming out of this setsockopt.  This is because
  // SO_EXCLUSIVEADDRUSE requires admin privileges on WinXP, but we don't
  // want to force socket pairs to be an admin.
  int reuse = 1;
  setsockopt(listener, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&reuse, sizeof(reuse));

  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_family = normalizeSockFamily(family);
  address.sin_port = 0;

  if (bind(listener, (struct sockaddr*)&address, sizeof(address)) != 0) {
    ret = -WSAGetLastError();
    NQPlatformSocketClose(listener);
    return ret;
  }

  if (getsockname(listener, (struct sockaddr*)&address, &size) != 0) {
    ret = -WSAGetLastError();
    NQPlatformSocketClose(listener);
    return ret;
  }

  ret = NQPlatformSocketListen(listener, 1);
  if (ret != 0) {
    NQPlatformSocketClose(listener);
    return ret;
  }

  ret = NQSocketOpen(family, type, protocol, &sock[0]);
  if (ret != 0) {
    NQPlatformSocketClose(listener);
    return ret;
  }

  ret = NQPlatformSocketConnect(sock[0], (struct sockaddr*)&address, sizeof(address));
  if (ret != 0) {
    NQPlatformSocketClose(sock[0]);
    NQPlatformSocketClose(listener);
    return ret;
  }

  ret = NQPlatformSocketAccept(listener, (struct sockaddr*)&address, &size, &sock[1]);
  if (ret != 0) {
    NQPlatformSocketClose(sock[0]);
    NQPlatformSocketClose(listener);
    return ret;
  }

  NQPlatformSocketClose(listener);
  return 0;
#else
  return -NQ_ENOTSUPP;

#endif
}
