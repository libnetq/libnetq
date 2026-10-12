/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"

#include <libnetq/Basic.h>

#if defined(NQ_OS_WINDOWS)
# include <winsock2.h>
# include <ws2tcpip.h>
# include <iphlpapi.h>
#endif

#include "libnetq/net/LocalAddress.h"

#include <libnetq/ErrorCode.h>
#include <libnetq/Malloc.h>

#if defined(NQ_OS_UNIX)
# include <ifaddrs.h>
# include <net/if.h>
# include <netinet/in.h>
# include <sys/socket.h>
#endif

/* Bionic declares getifaddrs()/freeifaddrs() only for API level 24+ */
#if defined(NQ_OS_ANDROID) && __ANDROID_API__ < 24
# define NQ_LOCALADDR_DYNAMIC_IFADDRS 1
# include <dlfcn.h>
# include <sys/ioctl.h>
# include <unistd.h>
#endif

#if defined(NQ_OS_WINDOWS) || defined(NQ_OS_UNIX)

/* Appends an IPv4 or IPv6 socket address unless it is link-local IPv6 */
static int append_address(const struct sockaddr* address, NQEndPoint** list, size_t* count)
{
  NQEndPoint endpoint;
  if (address->sa_family == AF_INET) {
    NQEndPoint_init4(&endpoint, (const uint8_t*)&((const struct sockaddr_in*)address)->sin_addr, 0);
  } else if (address->sa_family == AF_INET6) {
    const uint8_t* bytes = (const uint8_t*)&((const struct sockaddr_in6*)address)->sin6_addr;
    if (bytes[0] == 0xfe && (bytes[1] & 0xc0) == 0x80)
      return 0;
    NQEndPoint_init6(&endpoint, bytes, 0);
  } else {
    return 0;
  }

  NQEndPoint* grown = (NQEndPoint*)NQRealloc(*list, (*count + 1) * sizeof(NQEndPoint));
  if (grown == NULL)
    return -NQ_ENOMEM;
  grown[*count] = endpoint;
  *list = grown;
  (*count)++;
  return 0;
}

#endif

#if defined(NQ_LOCALADDR_DYNAMIC_IFADDRS)

typedef int (*getifaddrs_fn)(struct ifaddrs**);
typedef void (*freeifaddrs_fn)(struct ifaddrs*);

/* IPv4-only fallback for devices older than API 24 */
static int collect_ioctl(NQEndPoint** list, size_t* count)
{
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0)
    return -NQGetLastError();

  struct ifreq requests[64];
  struct ifconf conf;
  conf.ifc_len = (int)sizeof(requests);
  conf.ifc_req = requests;
  if (ioctl(fd, SIOCGIFCONF, &conf) < 0) {
    int err = -NQGetLastError();
    close(fd);
    return err;
  }

  int ret = 0;
  size_t n = (size_t)conf.ifc_len / sizeof(struct ifreq);
  for (size_t i = 0; i < n && ret == 0; ++i) {
    struct ifreq flags = requests[i];
    if (ioctl(fd, SIOCGIFFLAGS, &flags) < 0)
      continue;
    if (!(flags.ifr_flags & IFF_UP) || (flags.ifr_flags & IFF_LOOPBACK))
      continue;
    ret = append_address(&requests[i].ifr_addr, list, count);
  }
  close(fd);
  return ret;
}

#endif

int NQGetLocalAddresses(NQEndPoint** addresses, size_t* count)
{
  *addresses = NULL;
  *count = 0;
  int ret = 0;

#if defined(NQ_OS_WINDOWS)
  ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
  ULONG size = 16384;
  IP_ADAPTER_ADDRESSES* adapters = NULL;
  ULONG status;
  do {
    NQFree(adapters);
    adapters = (IP_ADAPTER_ADDRESSES*)NQMalloc(size);
    if (adapters == NULL)
      return -NQ_ENOMEM;
    status = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, adapters, &size);
  } while (status == ERROR_BUFFER_OVERFLOW);
  if (status != NO_ERROR) {
    NQFree(adapters);
    return status == ERROR_NO_DATA ? 0 : -(int)status;
  }

  for (IP_ADAPTER_ADDRESSES* a = adapters; a != NULL && ret == 0; a = a->Next) {
    if (a->OperStatus != IfOperStatusUp || a->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
      continue;
    for (IP_ADAPTER_UNICAST_ADDRESS* u = a->FirstUnicastAddress; u != NULL && ret == 0; u = u->Next)
      ret = append_address(u->Address.lpSockaddr, addresses, count);
  }
  NQFree(adapters);

#elif defined(NQ_OS_UNIX)
# if defined(NQ_LOCALADDR_DYNAMIC_IFADDRS)
  getifaddrs_fn getifaddrs = (getifaddrs_fn)dlsym(RTLD_DEFAULT, "getifaddrs");
  freeifaddrs_fn freeifaddrs = (freeifaddrs_fn)dlsym(RTLD_DEFAULT, "freeifaddrs");
  if (getifaddrs == NULL || freeifaddrs == NULL) {
    ret = collect_ioctl(addresses, count);
    goto done;
  }
# endif
  struct ifaddrs* interfaces = NULL;
  if (getifaddrs(&interfaces) != 0)
    return -NQGetLastError();
  for (struct ifaddrs* i = interfaces; i != NULL && ret == 0; i = i->ifa_next) {
    if (i->ifa_addr == NULL || !(i->ifa_flags & IFF_UP) || (i->ifa_flags & IFF_LOOPBACK))
      continue;
    ret = append_address(i->ifa_addr, addresses, count);
  }
  freeifaddrs(interfaces);

#else
  ret = -NQ_ENOTSUPP;
#endif

#if defined(NQ_LOCALADDR_DYNAMIC_IFADDRS)
done:
#endif
  if (ret != 0) {
    NQFree(*addresses);
    *addresses = NULL;
    *count = 0;
  }
  return ret;
}
