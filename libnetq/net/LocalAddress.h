/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_NET_LOCALADDRESS_H
#define _LIBNETQ_NET_LOCALADDRESS_H

#include <libnetq/net/EndPoint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The addresses of this machine that others can reach it at: the IPv4 and
 * IPv6 addresses of the interfaces that are up, leaving out the loopback
 * interfaces and IPv6 link-local addresses, in the order the system lists
 * them. Returns 0 and a list for NQFree in *addresses, NULL when *count is
 * 0, or the negated error code.
 */
NQ_EXPORT int NQGetLocalAddresses(NQEndPoint** addresses, size_t* count);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_NET_LOCALADDRESS_H */
