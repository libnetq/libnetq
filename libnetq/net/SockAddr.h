/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_NET_SOCKADDR_H
#define _LIBNETQ_NET_SOCKADDR_H

#include <libnetq/Basic.h>

#if defined(NQ_OS_KERNEL)
# include <uapi/linux/in.h> // for sockaddr_in
# include <uapi/linux/in6.h> // for sockaddr_in6
#elif defined(NQ_OS_UNIX)
# include <sys/socket.h>
#elif defined(NQ_OS_WINDOWS)
# include <ws2tcpip.h> // for sockaddr_in6
#endif

#if defined(NQ_OS_KERNEL) || defined(NQ_OS_UNIX) || defined(NQ_OS_WINDOWS)
typedef struct sockaddr NQSockAddr;
typedef struct sockaddr_in NQSockAddrIn;
typedef struct sockaddr_in6 NQSockAddrIn6;
typedef struct sockaddr_storage NQSockAddrStorage;
#else
typedef struct NQSockAddr NQSockAddr;
struct NQSockAddr {
  short sa_family;
  char sa_data[14];
};
struct NQInAddr {
  uint32_t s_addr;
};
typedef struct NQSockAddrIn NQSockAddrIn;
struct NQSockAddrIn {
  short sin_family;
  short sin_port;
  struct NQInAddr sin_addr;
  uint8_t sin_zero[8];
};
union NQIn6Addr {
  uint8_t s6_addr[16];
  uint16_t s6_addr16[8];
  uint32_t s6_addr32[4];
};
typedef struct NQSockAddrIn6 NQSockAddrIn6;
struct NQSockAddrIn6 {
  short sin6_family;
  short sin6_port;
  uint32_t sin6_flowinfo;
  union NQIn6Addr sin6_addr;
  uint32_t sin6_scope_id;
};
typedef struct NQSockAddrStorage NQSockAddrStorage;
struct NQSockAddrStorage {
  short ss_family;
  uint8_t __data[128 - sizeof(short) - sizeof(void*)];
  void* __align;
};
#endif

typedef union NQUnionSockAddr NQUnionSockAddr;
union NQUnionSockAddr {
  NQSockAddr sa;
  NQSockAddrIn in4;
  NQSockAddrIn6 in6;
  NQSockAddrStorage storage;
};

#endif /* _LIBNETQ_NET_SOCKADDR_H */
