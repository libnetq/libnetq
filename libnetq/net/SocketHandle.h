/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_NET_SOCKETHANDLE_H
#define _LIBNETQ_NET_SOCKETHANDLE_H

#include <libnetq/net/Netinet.h>
#include <libnetq/net/EndPoint.h>

#if defined(NQ_OS_KERNEL)
typedef struct socket* NQSocketHandle;
#elif defined(NQ_OS_UNIX)
typedef int NQSocketHandle;
#elif defined(NQ_OS_WINDOWS)
typedef uintptr_t NQSocketHandle;
#else
typedef void* NQSocketHandle;
#endif

#ifdef __cplusplus
extern "C" {
#endif

NQ_EXPORT int NQSocketOpen(int family, int type, int protocol, NQSocketHandle* result);
NQ_EXPORT void NQSocketClose(NQSocketHandle handle);
NQ_EXPORT int NQSocketSend(NQSocketHandle handle, const void* buf, size_t len, int flags);
NQ_EXPORT int NQSocketRecv(NQSocketHandle handle, void* buf, size_t len, int flags);

NQ_EXPORT int NQSocketGetOpt(NQSocketHandle handle, int level, int optname, void* optval, int* optlen);
NQ_EXPORT int NQSocketSetOpt(NQSocketHandle handle, int level, int optname, const void* optval, int optlen);

static inline int NQSocketGetOptBool(NQSocketHandle handle, int level, int optname, bool* value)
{
  int val;
  int len = sizeof(val);
  int ret = NQSocketGetOpt(handle, level, optname, &val, &len);
  if (ret == 0) {
    *value = val ? true : false;
  }
  return ret;
}

static inline int NQSocketSetOptBool(NQSocketHandle handle, int level, int optname, bool value)
{
  int val = value ? 1 : 0;
  return NQSocketSetOpt(handle, level, optname, &val, sizeof(val));
}

static inline int NQSocketGetOptInt(NQSocketHandle handle, int level, int optname, int* value)
{
  int len = sizeof(*value);
  return NQSocketGetOpt(handle, level, optname, value, &len);
}

static inline int NQSocketSetOptInt(NQSocketHandle handle, int level, int optname, int value)
{
  return NQSocketSetOpt(handle, level, optname, &value, sizeof(value));
}

static inline int NQSocketSetNoDelay(NQSocketHandle handle, bool value)
{
  return NQSocketSetOptBool(handle, NQ_IPPROTO_TCP, NQ_TCP_NODELAY, value);
}

NQ_EXPORT int NQSocketShutdown(NQSocketHandle handle, int how);
NQ_EXPORT int NQSocketSetNonBlocking(NQSocketHandle handle, bool value);
NQ_EXPORT bool NQSocketIsSelectable(NQSocketHandle handle);

NQ_EXPORT int NQSocketConnect(NQSocketHandle handle, const NQEndPoint* ep);
NQ_EXPORT int NQSocketConnect4(NQSocketHandle handle, const NQIPv4EndPoint* ep);
NQ_EXPORT int NQSocketConnect6(NQSocketHandle handle, const NQIPv6EndPoint* ep);
NQ_EXPORT int NQSocketAccept(NQSocketHandle handle, NQEndPoint* ep, NQSocketHandle* result);
NQ_EXPORT int NQSocketBind(NQSocketHandle handle, const NQEndPoint* ep);
NQ_EXPORT int NQSocketBind4(NQSocketHandle handle, const NQIPv4EndPoint* ep);
NQ_EXPORT int NQSocketBind6(NQSocketHandle handle, const NQIPv6EndPoint* ep);
NQ_EXPORT int NQSocketListen(NQSocketHandle handle, int backlog);

NQ_EXPORT int NQSocketSendTo(NQSocketHandle handle, const uint8_t* buf, size_t len, int flags, const NQEndPoint* ep);
NQ_EXPORT int NQSocketSendTo4(NQSocketHandle handle, const uint8_t* buf, size_t len, int flags, const NQIPv4EndPoint* ep);
NQ_EXPORT int NQSocketSendTo6(NQSocketHandle handle, const uint8_t* buf, size_t len, int flags, const NQIPv6EndPoint* ep);

NQ_EXPORT int NQSocketPair(int family, int type, int protocol, NQSocketHandle socks[2]);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_NET_SOCKETHANDLE_H */
