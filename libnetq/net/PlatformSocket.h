/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_NET_PLATFORMSOCKET_H
#define _LIBNETQ_NET_PLATFORMSOCKET_H

#include <libnetq/Basic.h>

#if defined(NQ_OS_KERNEL)
# include <linux/socket.h>
# include <uapi/linux/in.h> // for sockaddr_in
# include <uapi/linux/in6.h> // for sockaddr_in6
#elif defined(NQ_OS_UNIX)
# include <sys/socket.h>
# include <unistd.h> // for close
#elif defined(NQ_OS_WINDOWS)
# include <winsock2.h>
# include <ws2tcpip.h> // for sockaddr_in6
#endif

#include <libnetq/Assert.h>
#include <libnetq/Limits.h>
#include <libnetq/MinMax.h>
#include <libnetq/ErrorCode.h>

typedef struct sockaddr NQSockAddr;
typedef struct sockaddr_in NQSockAddrIn;
typedef struct sockaddr_in6 NQSockAddrIn6;
typedef struct sockaddr_storage NQSockAddrStorage;

typedef union NQUnionSockAddr NQUnionSockAddr;
union NQUnionSockAddr {
  NQSockAddr sa;
  NQSockAddrIn in4;
  NQSockAddrIn6 in6;
  NQSockAddrStorage storage;
};

#if defined(NQ_OS_KERNEL)

typedef struct socket* NQPlatformSocket;

static inline int NQPlatformSocketOpen(int family, int type, int protocol, NQPlatformSocket* result)
{
  return sock_create_lite(family, type, protocol, result);
}

static inline void NQPlatformSocketClose(NQPlatformSocket sock)
{
  sock_release(sock);
}

static inline int NQPlatformSocketSend(NQPlatformSocket sock, const void* buf, size_t len, int flags)
{
  NQ_UNUSED_PARAM(sock);
  NQ_UNUSED_PARAM(buf);
  NQ_UNUSED_PARAM(len);
  NQ_UNUSED_PARAM(flags);

  return -NQ_ENOTSUPP;
}

static inline int NQPlatformSocketRecv(NQPlatformSocket sock, void* buf, size_t len, int flags)
{
  NQ_UNUSED_PARAM(sock);
  NQ_UNUSED_PARAM(buf);
  NQ_UNUSED_PARAM(len);
  NQ_UNUSED_PARAM(flags);

  return -NQ_ENOTSUPP;
}

static inline int NQPlatformSocketConnect(NQPlatformSocket sock, const NQSockAddr* addr, int addrlen)
{
  NQ_UNUSED_PARAM(sock);
  NQ_UNUSED_PARAM(addr);
  NQ_UNUSED_PARAM(addrlen);

  return -NQ_ENOTSUPP;
}

static inline int NQPlatformSocketAccept(NQPlatformSocket sock, NQSockAddr* addr, int* addrlen, NQPlatformSocket* result)
{
  NQ_UNUSED_PARAM(sock);
  NQ_UNUSED_PARAM(addr);
  NQ_UNUSED_PARAM(addrlen);
  NQ_UNUSED_PARAM(result);

  return -NQ_ENOTSUPP;
}

static inline int NQPlatformSocketListen(NQPlatformSocket sock, int backlog)
{
  NQ_UNUSED_PARAM(sock);
  NQ_UNUSED_PARAM(backlog);

  return -NQ_ENOTSUPP;
}

static inline int NQPlatformSocketBind(NQPlatformSocket sock, const NQSockAddr* addr, int addrlen)
{
  NQ_UNUSED_PARAM(sock);
  NQ_UNUSED_PARAM(addr);
  NQ_UNUSED_PARAM(addrlen);

  return -NQ_ENOTSUPP;
}

static inline int NQPlatformSocketGetOpt(NQPlatformSocket sock, int level, int optname, void* optval, int* optlen)
{
  NQ_UNUSED_PARAM(sock);
  NQ_UNUSED_PARAM(level);
  NQ_UNUSED_PARAM(optname);
  NQ_UNUSED_PARAM(optval);
  NQ_UNUSED_PARAM(optlen);

  return -NQ_ENOTSUPP;
}

static inline int NQPlatformSocketSetOpt(NQPlatformSocket sock, int level, int optname, const void* optval, int optlen)
{
  NQ_UNUSED_PARAM(sock);
  NQ_UNUSED_PARAM(level);
  NQ_UNUSED_PARAM(optname);
  NQ_UNUSED_PARAM(optval);
  NQ_UNUSED_PARAM(optlen);

  return -NQ_ENOTSUPP;
}

static inline int NQPlatformSocketSendto(NQPlatformSocket sock, const void* buf, size_t len, int flags, const NQSockAddr* addr, int addrlen)
{
  NQ_UNUSED_PARAM(sock);
  NQ_UNUSED_PARAM(buf);
  NQ_UNUSED_PARAM(len);
  NQ_UNUSED_PARAM(flags);
  NQ_UNUSED_PARAM(addr);
  NQ_UNUSED_PARAM(addrlen);

  return -NQ_ENOTSUPP;
}

static inline int NQPlatformSocketShutdown(NQPlatformSocket sock, int how)
{
  return kernel_sock_shutdown(sock, how);
}

#elif defined(NQ_OS_UNIX)

typedef int NQPlatformSocket;

static inline int NQPlatformSocketOpen(int family, int type, int protocol, NQPlatformSocket* result)
{
  NQPlatformSocket sock = socket(family, type, protocol);
  if (sock == -1)
    return -NQGetLastError();
  *result = sock;
  return 0;
}

static inline void NQPlatformSocketClose(NQPlatformSocket sock)
{
  close(sock);
}

static inline int NQPlatformSocketSend(NQPlatformSocket sock, const void* buf, size_t len, int flags)
{
  NQ_ASSERT(len <= NQ_INT32_MAX);
#ifdef NQ_OS_LINUX
  flags |= MSG_NOSIGNAL;
#endif
  ssize_t ret = send(sock, buf, NQGetMin(len, NQ_INT32_MAX), flags);
  if (ret == -1)
    return -NQGetLastError();
  return (int)ret;
}

static inline int NQPlatformSocketRecv(NQPlatformSocket sock, void* buf, size_t len, int flags)
{
  NQ_ASSERT(len <= NQ_INT32_MAX);
#ifdef NQ_OS_LINUX
  flags |= MSG_NOSIGNAL;
#endif
  // flags |= MSG_DONTWAIT; // TODO
  ssize_t ret = recv(sock, buf, NQGetMin(len, NQ_INT32_MAX), flags);
  if (ret == -1)
    return -NQGetLastError();
  return (int)ret;
}

static inline int NQPlatformSocketConnect(NQPlatformSocket sock, const NQSockAddr* addr, int addrlen)
{
  int ret = connect(sock, addr, (socklen_t)addrlen);
  if (ret == -1)
    return -NQGetLastError();
  return ret;
}

static inline int NQPlatformSocketAccept(NQPlatformSocket sock, NQSockAddr* addr, int* addrlen, NQPlatformSocket* result)
{
  int ret;

  if (sizeof(socklen_t) == sizeof(*addrlen))
    ret = accept(sock, addr, (socklen_t*)addrlen);
  else {
    socklen_t temp = (socklen_t)(*addrlen);
    ret = accept(sock, addr, &temp);
    *addrlen = (int)temp;
  }

  if (ret == -1)
    return -NQGetLastError();

  *result = ret;
  return 0;
}

static inline int NQPlatformSocketListen(NQPlatformSocket sock, int backlog)
{
  int ret = listen(sock, backlog);
  if (ret == -1)
    return -NQGetLastError();
  return ret;
}

static inline int NQPlatformSocketBind(NQPlatformSocket sock, const NQSockAddr* addr, int addrlen)
{
  int ret = bind(sock, addr, (socklen_t)addrlen);
  if (ret == -1)
    return -NQGetLastError();
  return ret;
}

static inline int NQPlatformSocketGetOpt(NQPlatformSocket sock, int level, int optname, void* optval, int* optlen)
{
  int ret;

  if (sizeof(socklen_t) == sizeof(*optlen))
    ret = getsockopt(sock, level, optname, optval, (socklen_t*)optlen);
  else {
    socklen_t temp = (socklen_t)*optlen;
    ret = getsockopt(sock, level, optname, optval, &temp);
    *optlen = temp;
  }

  if (ret == -1)
    return -NQGetLastError();

  return ret;
}

static inline int NQPlatformSocketSetOpt(NQPlatformSocket sock, int level, int optname, const void* optval, int optlen)
{
  int ret = setsockopt(sock, level, optname, optval, (socklen_t)optlen);
  if (ret == -1)
    return -NQGetLastError();
  return ret;
}

static inline int NQPlatformSocketSendto(NQPlatformSocket sock, const void* buf, size_t len, int flags, const NQSockAddr* addr, int addrlen)
{
  NQ_ASSERT(len <= NQ_INT32_MAX);
#ifdef NQ_OS_LINUX
  flags |= MSG_NOSIGNAL;
#endif
  ssize_t ret = sendto(sock, buf, NQGetMin(len, NQ_INT32_MAX), flags, addr, (socklen_t)addrlen);
  if (ret == -1)
    return -NQGetLastError();
  return (int)ret;
}

static inline int NQPlatformSocketShutdown(NQPlatformSocket sock, int how)
{
  int ret = shutdown(sock, how);
  if (ret == -1)
    return -NQGetLastError();
  return ret;
}

#elif defined(NQ_OS_WINDOWS)

typedef SOCKET NQPlatformSocket;

static inline int NQPlatformSocketOpen(int family, int type, int protocol, NQPlatformSocket* result)
{
  NQPlatformSocket sock = WSASocketW(family, type, protocol, NULL, 0, 0);
  if (sock == INVALID_SOCKET)
    return -WSAGetLastError();
  *result = sock;
  return 0;
}

static inline void NQPlatformSocketClose(NQPlatformSocket sock)
{
  closesocket(sock);
}

static inline int NQPlatformSocketSend(NQPlatformSocket sock, const void* buf, size_t len, int flags)
{
  NQ_ASSERT(len <= NQ_INT32_MAX);
  int ret = send(sock, (const char*)buf, (int)NQGetMin(len, NQ_INT32_MAX), flags);
  if (ret == SOCKET_ERROR)
    return -WSAGetLastError();
  return ret;
}

static inline int NQPlatformSocketRecv(NQPlatformSocket sock, void* buf, size_t len, int flags)
{
  NQ_ASSERT(len <= NQ_INT32_MAX);
  int ret = recv(sock, (char*)buf, (int)NQGetMin(len, NQ_INT32_MAX), flags);
  if (ret == SOCKET_ERROR)
    return -WSAGetLastError();
  return ret;
}

static inline int NQPlatformSocketConnect(NQPlatformSocket sock, const NQSockAddr* addr, int addrlen)
{
  int ret = WSAConnect(sock, addr, addrlen, NULL, NULL, NULL, NULL);
  if (ret == SOCKET_ERROR)
    return -WSAGetLastError();
  return ret;
}

static inline int NQPlatformSocketAccept(NQPlatformSocket sock, NQSockAddr* addr, int* addrlen, NQPlatformSocket* result)
{
  NQPlatformSocket ret = WSAAccept(sock, addr, addrlen, NULL, 0);
  if (ret == INVALID_SOCKET)
    return -WSAGetLastError();
  *result = ret;
  return 0;
}

static inline int NQPlatformSocketListen(NQPlatformSocket sock, int backlog)
{
  int ret = listen(sock, backlog);
  if (ret == SOCKET_ERROR)
    return -WSAGetLastError();
  return ret;
}

static inline int NQPlatformSocketBind(NQPlatformSocket sock, const NQSockAddr* addr, int addrlen)
{
  int ret = bind(sock, addr, addrlen);
  if (ret == SOCKET_ERROR)
    return -WSAGetLastError();
  return ret;
}

static inline int NQPlatformSocketGetOpt(NQPlatformSocket sock, int level, int optname, void* optval, int* optlen)
{
  int ret = getsockopt(sock, level, optname, (char*)optval, optlen);
  if (ret == SOCKET_ERROR)
    return -WSAGetLastError();
  return ret;
}

static inline int NQPlatformSocketSetOpt(NQPlatformSocket sock, int level, int optname, const void* optval, int optlen)
{
  int ret = setsockopt(sock, level, optname, (const char*)optval, optlen);
  if (ret == SOCKET_ERROR)
    return -WSAGetLastError();
  return ret;
}

static inline int NQPlatformSocketSendto(NQPlatformSocket sock, const void* buf, size_t len, int flags, const NQSockAddr* addr, int addrlen)
{
  NQ_ASSERT(len <= NQ_INT32_MAX);
  int ret = sendto(sock, (const char*)buf, (int)NQGetMin(len, NQ_INT32_MAX), flags, addr, addrlen);
  if (ret == SOCKET_ERROR)
    return -WSAGetLastError();
  return ret;
}

static inline int NQPlatformSocketShutdown(NQPlatformSocket sock, int how)
{
  int ret = shutdown(sock, how);
  if (ret == SOCKET_ERROR)
    return -WSAGetLastError();
  return ret;
}

#else

typedef void* NQPlatformSocket;

static inline int NQPlatformSocketOpen(int family, int type, int protocol, NQPlatformSocket* result)
{
  NQ_UNUSED_PARAM(family);
  NQ_UNUSED_PARAM(type);
  NQ_UNUSED_PARAM(protocol);
  NQ_UNUSED_PARAM(result);

  return -NQ_ENOTSUPP;
}

static inline void NQPlatformSocketClose(NQPlatformSocket sock)
{
  NQ_UNUSED_PARAM(sock);
}

static inline int NQPlatformSocketSend(NQPlatformSocket sock, const void* buf, size_t len, int flags)
{
  NQ_UNUSED_PARAM(sock);
  NQ_UNUSED_PARAM(buf);
  NQ_UNUSED_PARAM(len);
  NQ_UNUSED_PARAM(flags);

  return -NQ_ENOTSUPP;
}

static inline int NQPlatformSocketRecv(NQPlatformSocket sock, void* buf, size_t len, int flags)
{
  NQ_UNUSED_PARAM(sock);
  NQ_UNUSED_PARAM(buf);
  NQ_UNUSED_PARAM(len);
  NQ_UNUSED_PARAM(flags);

  return -NQ_ENOTSUPP;
}

static inline int NQPlatformSocketConnect(NQPlatformSocket sock, const NQSockAddr* addr, int addrlen)
{
  NQ_UNUSED_PARAM(sock);
  NQ_UNUSED_PARAM(addr);
  NQ_UNUSED_PARAM(addrlen);

  return -NQ_ENOTSUPP;
}

static inline int NQPlatformSocketAccept(NQPlatformSocket sock, NQSockAddr* addr, int* addrlen, NQPlatformSocket* result)
{
  NQ_UNUSED_PARAM(sock);
  NQ_UNUSED_PARAM(addr);
  NQ_UNUSED_PARAM(addrlen);
  NQ_UNUSED_PARAM(result);

  return -NQ_ENOTSUPP;
}

static inline int NQPlatformSocketListen(NQPlatformSocket sock, int backlog)
{
  NQ_UNUSED_PARAM(sock);
  NQ_UNUSED_PARAM(backlog);

  return -NQ_ENOTSUPP;
}

static inline int NQPlatformSocketBind(NQPlatformSocket sock, const NQSockAddr* addr, int addrlen)
{
  NQ_UNUSED_PARAM(sock);
  NQ_UNUSED_PARAM(addr);
  NQ_UNUSED_PARAM(addrlen);

  return -NQ_ENOTSUPP;
}

static inline int NQPlatformSocketGetOpt(NQPlatformSocket sock, int level, int optname, void* optval, int* optlen)
{
  NQ_UNUSED_PARAM(sock);
  NQ_UNUSED_PARAM(level);
  NQ_UNUSED_PARAM(optname);
  NQ_UNUSED_PARAM(optval);
  NQ_UNUSED_PARAM(optlen);

  return -NQ_ENOTSUPP;
}

static inline int NQPlatformSocketSetOpt(NQPlatformSocket sock, int level, int optname, const void* optval, int optlen)
{
  NQ_UNUSED_PARAM(sock);
  NQ_UNUSED_PARAM(level);
  NQ_UNUSED_PARAM(optname);
  NQ_UNUSED_PARAM(optval);
  NQ_UNUSED_PARAM(optlen);

  return -NQ_ENOTSUPP;
}

static inline int NQPlatformSocketSendto(NQPlatformSocket sock, const void* buf, size_t len, int flags, const NQSockAddr* addr, int addrlen)
{
  NQ_UNUSED_PARAM(sock);
  NQ_UNUSED_PARAM(buf);
  NQ_UNUSED_PARAM(len);
  NQ_UNUSED_PARAM(flags);
  NQ_UNUSED_PARAM(addr);
  NQ_UNUSED_PARAM(addrlen);

  return -NQ_ENOTSUPP;
}

static inline int NQPlatformSocketShutdown(NQPlatformSocket sock, int how)
{
  NQ_UNUSED_PARAM(sock);
  NQ_UNUSED_PARAM(how);

  return -NQ_ENOTSUPP;
}

#endif

#endif /* _LIBNETQ_NET_PLATFORMSOCKET_H */
