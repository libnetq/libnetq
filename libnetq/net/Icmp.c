/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"

#if defined(_WIN32)
# include <winsock2.h>
# include <ws2tcpip.h>
# include <iphlpapi.h>
# include <icmpapi.h>
#endif

#include "libnetq/net/Icmp.h"

#include <libnetq/ErrorCode.h>
#include <libnetq/Malloc.h>
#include <libnetq/net/Socket.h>
#include <libnetq/string/String.h>

#if defined(NQ_OS_WINDOWS)
# include <libnetq/backend/win32/Socket.h>
#elif defined(NQ_OS_UNIX)
# include <errno.h>
# include <fcntl.h>
# include <netinet/in.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <sys/uio.h>
# include <unistd.h>
# if defined(NQ_OS_LINUX)
#  include <linux/errqueue.h>
# endif
#endif

#define ICMP_ECHO_REQUEST   8
#define ICMP_ECHO_REPLY_    0
#define ICMP6_ECHO_REQUEST  128
#define ICMP6_ECHO_REPLY_   129

#if defined(NQ_OS_UNIX)

/* The largest ICMP message read: an IP datagram */
#define ICMP_BUFFER 65536

struct NQIcmpSocket {
  int fd;
  bool v6;
  bool raw;
  struct sockaddr_storage to;
  socklen_t toLen;
  uint8_t* buffer;
};

static socklen_t to_sockaddr(const NQEndPoint* ep, struct sockaddr_storage* ss)
{
  NQMemset(ss, 0, sizeof(*ss));
  if (ep->family == NQ_AF_INET6) {
    struct sockaddr_in6* in6 = (struct sockaddr_in6*)ss;
    in6->sin6_family = AF_INET6;
    NQMemcpy(&in6->sin6_addr, ep->ip6ep.address.data, 16);
    return sizeof(*in6);
  }
  struct sockaddr_in* in4 = (struct sockaddr_in*)ss;
  in4->sin_family = AF_INET;
  NQMemcpy(&in4->sin_addr, ep->ip4ep.address.data, 4);
  return sizeof(*in4);
}

static void from_sockaddr(const struct sockaddr_storage* ss, NQEndPoint* ep)
{
  if (ss->ss_family == AF_INET6)
    NQEndPoint_init6(ep, (const uint8_t*)&((const struct sockaddr_in6*)ss)->sin6_addr, 0);
  else
    NQEndPoint_init4(ep, (const uint8_t*)&((const struct sockaddr_in*)ss)->sin_addr, 0);
}

int NQIcmpSocket_open(int family, NQIcmpSocket** result)
{
  *result = NULL;
  bool v6 = family == NQ_AF_INET6;
  int af = v6 ? AF_INET6 : AF_INET;
  int proto = v6 ? IPPROTO_ICMPV6 : IPPROTO_ICMP;

  /* a ping socket where the system allows one, else a raw socket */
  bool raw = false;
  int fd = socket(af, SOCK_DGRAM, proto);
  if (fd < 0) {
    int first = errno;
    fd = socket(af, SOCK_RAW, proto);
    if (fd < 0)
      return -(errno == EPERM || errno == EACCES ? errno : first);
    raw = true;
  }
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
  fcntl(fd, F_SETFD, FD_CLOEXEC);

  int on = 1;
  if (!v6) {
# ifdef IP_RECVTTL
    setsockopt(fd, IPPROTO_IP, IP_RECVTTL, &on, sizeof(on));
# endif
# if defined(NQ_OS_LINUX) && defined(IP_RECVERR)
    /* a ping socket hears of errors through its error queue */
    if (!raw)
      setsockopt(fd, IPPROTO_IP, IP_RECVERR, &on, sizeof(on));
# endif
  } else {
# ifdef IPV6_RECVHOPLIMIT
    setsockopt(fd, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &on, sizeof(on));
# endif
# if defined(NQ_OS_LINUX) && defined(IPV6_RECVERR)
    if (!raw)
      setsockopt(fd, IPPROTO_IPV6, IPV6_RECVERR, &on, sizeof(on));
# endif
  }

  NQIcmpSocket* s = NQZeroMalloc(sizeof(*s));
  uint8_t* buffer = NQMalloc(ICMP_BUFFER);
  if (!s || !buffer) {
    NQFree(s);
    NQFree(buffer);
    close(fd);
    return -NQ_ENOMEM;
  }
  s->fd = fd;
  s->v6 = v6;
  s->raw = raw;
  s->buffer = buffer;
  *result = s;
  return 0;
}

void NQIcmpSocket_close(NQIcmpSocket* s)
{
  if (!s)
    return;
  close(s->fd);
  NQFree(s->buffer);
  NQFree(s);
}

intptr_t NQIcmpSocket_handle(NQIcmpSocket* s)
{
  return s->fd;
}

bool NQIcmpSocket_isRaw(NQIcmpSocket* s)
{
  return s->raw;
}

bool NQIcmpSocket_ownsReplies(NQIcmpSocket* s)
{
  /* Linux gives a ping socket its own identifier and only its replies */
# if defined(NQ_OS_LINUX)
  return !s->raw;
# else
  NQ_UNUSED_PARAM(s);
  return false;
# endif
}

int NQIcmpSocket_setTtl(NQIcmpSocket* s, int ttl)
{
  int rc = s->v6 ? setsockopt(s->fd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &ttl, sizeof(ttl))
                 : setsockopt(s->fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
  return rc == 0 ? 0 : -errno;
}

int NQIcmpSocket_connect(NQIcmpSocket* s, const NQEndPoint* to)
{
  s->toLen = to_sockaddr(to, &s->to);
  /* as ping does: a UDP socket finds whether there is a route at all, and
   * the ICMP one stays unconnected, to hear errors from routers on the way */
  int probe = socket(s->to.ss_family, SOCK_DGRAM, 0);
  if (probe < 0)
    return 0;
  struct sockaddr_storage ss = s->to;
  if (ss.ss_family == AF_INET6)
    ((struct sockaddr_in6*)&ss)->sin6_port = htons(1025);
  else
    ((struct sockaddr_in*)&ss)->sin_port = htons(1025);
  int rc = connect(probe, (struct sockaddr*)&ss, s->toLen) == 0 ? 0 : -errno;
  close(probe);
  return rc;
}

static uint16_t checksum(const uint8_t* p, size_t n)
{
  uint32_t sum = 0;
  for (; n > 1; p += 2, n -= 2)
    sum += (uint32_t)(p[0] << 8 | p[1]);
  if (n)
    sum += (uint32_t)p[0] << 8;
  while (sum >> 16)
    sum = (sum & 0xffff) + (sum >> 16);
  return (uint16_t)~sum;
}

int NQIcmpSocket_sendEcho(NQIcmpSocket* s, uint16_t id, uint16_t seq, const void* data, size_t len)
{
  if (len > ICMP_BUFFER - 8)
    return -NQ_EINVAL;
  uint8_t* p = NQMalloc(8 + len);
  if (!p)
    return -NQ_ENOMEM;
  p[0] = s->v6 ? ICMP6_ECHO_REQUEST : ICMP_ECHO_REQUEST;
  p[1] = 0;
  p[2] = p[3] = 0;
  p[4] = (uint8_t)(id >> 8);
  p[5] = (uint8_t)id;
  p[6] = (uint8_t)(seq >> 8);
  p[7] = (uint8_t)seq;
  if (len)
    NQMemcpy(p + 8, data, len);
  if (!s->v6) {
    /* ICMPv6's the kernel computes, with the addresses it covers */
    uint16_t sum = checksum(p, 8 + len);
    p[2] = (uint8_t)(sum >> 8);
    p[3] = (uint8_t)sum;
  }
  ssize_t n = sendto(s->fd, p, 8 + len, 0, (struct sockaddr*)&s->to, s->toLen);
  int err = errno;
  NQFree(p);
  return n < 0 ? -err : 0;
}

/* An echo reply, or an error that quotes one of our echo requests */
static bool parse_icmp(bool v6, const uint8_t* p, size_t len, NQIcmpReply* r)
{
  if (len < 8)
    return false;
  r->type = p[0];
  r->code = p[1];
  r->length = len;
  if (p[0] == (v6 ? ICMP6_ECHO_REPLY_ : ICMP_ECHO_REPLY_)) {
    r->kind = NQ_ICMP_ECHO_REPLY;
    r->id = (uint16_t)(p[4] << 8 | p[5]);
    r->seq = (uint16_t)(p[6] << 8 | p[7]);
    return true;
  }
  bool error = v6 ? (p[0] >= 1 && p[0] <= 4) : (p[0] == 3 || p[0] == 4 || p[0] == 5 || p[0] == 11 || p[0] == 12);
  if (!error)
    return false;

  /* the datagram that caused it follows the header */
  const uint8_t* inner = p + 8;
  size_t ilen = len - 8;
  if (!v6) {
    if (ilen < 20 || inner[9] != IPPROTO_ICMP)
      return false;
    size_t ihl = (size_t)(inner[0] & 15) * 4;
    if (ilen < ihl)
      return false;
    inner += ihl;
    ilen -= ihl;
  } else {
    if (ilen < 40 || inner[6] != IPPROTO_ICMPV6)
      return false;
    inner += 40;
    ilen -= 40;
  }
  if (ilen < 8 || inner[0] != (v6 ? ICMP6_ECHO_REQUEST : ICMP_ECHO_REQUEST))
    return false;
  r->kind = NQ_ICMP_ERROR;
  r->id = (uint16_t)(inner[4] << 8 | inner[5]);
  r->seq = (uint16_t)(inner[6] << 8 | inner[7]);
  if (!v6 && p[0] == 3 && p[1] == 4)
    r->info = (uint32_t)(p[6] << 8 | p[7]);
  else if (v6 && p[0] == 2)
    r->info = (uint32_t)p[4] << 24 | (uint32_t)p[5] << 16 | (uint32_t)p[6] << 8 | p[7];
  return true;
}

# if defined(NQ_OS_LINUX)

/* An error queued for a ping socket: our echo request back, with what an
 * ICMP message said of it */
static int receive_error(NQIcmpSocket* s, NQIcmpReply* r)
{
  for (;;) {
    uint8_t data[1024];
    char control[512];
    struct sockaddr_storage from;
    struct iovec iov = { data, sizeof(data) };
    struct msghdr msg;
    NQMemset(&msg, 0, sizeof(msg));
    msg.msg_name = &from;
    msg.msg_namelen = sizeof(from);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);
    ssize_t n = recvmsg(s->fd, &msg, MSG_ERRQUEUE | MSG_DONTWAIT);
    if (n < 0)
      return 0;

    struct sock_extended_err* ee = NULL;
    for (struct cmsghdr* c = CMSG_FIRSTHDR(&msg); c; c = CMSG_NXTHDR(&msg, c)) {
      if ((c->cmsg_level == SOL_IP && c->cmsg_type == IP_RECVERR)
          || (c->cmsg_level == SOL_IPV6 && c->cmsg_type == IPV6_RECVERR))
        ee = (struct sock_extended_err*)CMSG_DATA(c);
    }
    if (!ee || (ee->ee_origin != SO_EE_ORIGIN_ICMP && ee->ee_origin != SO_EE_ORIGIN_ICMP6) || n < 8)
      continue; /* a local error: the send reported it */

    NQMemset(r, 0, sizeof(*r));
    r->kind = NQ_ICMP_ERROR;
    r->type = ee->ee_type;
    r->code = ee->ee_code;
    r->info = ee->ee_info;
    r->ttl = -1;
    r->length = (size_t)n;
    r->id = (uint16_t)(data[4] << 8 | data[5]);
    r->seq = (uint16_t)(data[6] << 8 | data[7]);
    struct sockaddr_storage offender;
    NQMemset(&offender, 0, sizeof(offender));
    NQMemcpy(&offender, SO_EE_OFFENDER(ee), s->v6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in));
    from_sockaddr(&offender, &r->from);
    return 1;
  }
}

# endif

int NQIcmpSocket_receive(NQIcmpSocket* s, NQIcmpReply* r)
{
# if defined(NQ_OS_LINUX)
  if (!s->raw && receive_error(s, r) == 1)
    return 1;
# endif
  for (;;) {
    char control[512];
    struct sockaddr_storage from;
    struct iovec iov = { s->buffer, ICMP_BUFFER };
    struct msghdr msg;
    NQMemset(&msg, 0, sizeof(msg));
    msg.msg_name = &from;
    msg.msg_namelen = sizeof(from);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);
    ssize_t n = recvmsg(s->fd, &msg, MSG_DONTWAIT);
    if (n < 0)
      return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ? 0 : -errno;

    NQMemset(r, 0, sizeof(*r));
    r->ttl = -1;
    from_sockaddr(&from, &r->from);
    for (struct cmsghdr* c = CMSG_FIRSTHDR(&msg); c; c = CMSG_NXTHDR(&msg, c)) {
      if (c->cmsg_level == IPPROTO_IP && (c->cmsg_type == IP_TTL
# ifdef IP_RECVTTL
                                          || c->cmsg_type == IP_RECVTTL
# endif
                                          )) {
        /* an int on Linux, a byte on the BSDs and macOS */
        r->ttl = c->cmsg_len >= CMSG_LEN(sizeof(int)) ? *(int*)CMSG_DATA(c) : *(uint8_t*)CMSG_DATA(c);
      } else if (c->cmsg_level == IPPROTO_IPV6 && c->cmsg_type == IPV6_HOPLIMIT) {
        r->ttl = *(int*)CMSG_DATA(c);
      }
    }

    /* raw IPv4 sockets, and macOS's ping sockets, keep the IP header */
    const uint8_t* p = s->buffer;
    size_t len = (size_t)n;
    if (!s->v6 && len >= 20 && (p[0] >> 4) == 4) {
      size_t ihl = (size_t)(p[0] & 15) * 4;
      if (len < ihl)
        continue;
      r->ttl = p[8];
      p += ihl;
      len -= ihl;
    }
    if (parse_icmp(s->v6, p, len, r))
      return 1;
  }
}

#elif defined(NQ_OS_WINDOWS) && !(defined(__MINGW64_VERSION_MAJOR) && __MINGW64_VERSION_MAJOR < 8)
/* mingw-w64 before 8 has no ICMP calls in libiphlpapi.a: it gets the stubs */

/* How long Windows waits for a reply before it gives the request up */
#define ICMP_TIMEOUT_MS 10000

/* ICMPV6_ECHO_REPLY, which older mingw-w64 headers lack: the address is
   packed as IPV6_ADDRESS_EX is */
#pragma pack(push, 1)
typedef struct {
  USHORT sin6_port;
  ULONG sin6_flowinfo;
  USHORT sin6_addr[8];
  ULONG sin6_scope_id;
} NQIcmp6Address;
#pragma pack(pop)

typedef struct {
  NQIcmp6Address Address;
  ULONG Status;
  unsigned int RoundTripTime;
} NQIcmp6EchoReply;

struct NQIcmpSocket {
  HANDLE icmp;
  HANDLE event;
  bool v6;
  bool pending;
  uint16_t id, seq;
  size_t len;
  int ttl;
  NQEndPoint to;
  void* reply;
  DWORD replySize;
  void* request;
};

int NQIcmpSocket_open(int family, NQIcmpSocket** result)
{
  *result = NULL;
  NQWinSocketInitialize();
  NQIcmpSocket* s = NQZeroMalloc(sizeof(*s));
  if (!s)
    return -NQ_ENOMEM;
  s->v6 = family == NQ_AF_INET6;
  s->icmp = s->v6 ? Icmp6CreateFile() : IcmpCreateFile();
  if (s->icmp == INVALID_HANDLE_VALUE) {
    int err = (int)GetLastError();
    NQFree(s);
    return -err;
  }
  s->event = CreateEventW(NULL, TRUE, FALSE, NULL);
  if (!s->event) {
    int err = (int)GetLastError();
    IcmpCloseHandle(s->icmp);
    NQFree(s);
    return -err;
  }
  *result = s;
  return 0;
}

void NQIcmpSocket_close(NQIcmpSocket* s)
{
  if (!s)
    return;
  IcmpCloseHandle(s->icmp);
  /* a request still out writes into its buffers until it ends */
  if (s->pending)
    WaitForSingleObject(s->event, ICMP_TIMEOUT_MS);
  CloseHandle(s->event);
  NQFree(s->reply);
  NQFree(s->request);
  NQFree(s);
}

intptr_t NQIcmpSocket_handle(NQIcmpSocket* s)
{
  return (intptr_t)s->event;
}

bool NQIcmpSocket_isRaw(NQIcmpSocket* s)
{
  NQ_UNUSED_PARAM(s);
  return false;
}

bool NQIcmpSocket_ownsReplies(NQIcmpSocket* s)
{
  NQ_UNUSED_PARAM(s);
  return true; /* each request gets its own reply */
}

int NQIcmpSocket_setTtl(NQIcmpSocket* s, int ttl)
{
  s->ttl = ttl;
  return 0;
}

int NQIcmpSocket_connect(NQIcmpSocket* s, const NQEndPoint* to)
{
  s->to = *to;
  return 0;
}

int NQIcmpSocket_sendEcho(NQIcmpSocket* s, uint16_t id, uint16_t seq, const void* data, size_t len)
{
  if (s->pending)
    return -NQ_EBUSY; /* one request at a time */
  if (len > 65500)
    return -NQ_EINVAL;
  DWORD size = (DWORD)((s->v6 ? sizeof(NQIcmp6EchoReply) : sizeof(ICMP_ECHO_REPLY)) + len + 8 + 64);
  if (size > s->replySize) {
    void* more = NQRealloc(s->reply, size);
    if (!more)
      return -NQ_ENOMEM;
    s->reply = more;
    s->replySize = size;
  }
  /* the data must stay until the request ends */
  void* request = NQRealloc(s->request, len ? len : 1);
  if (!request)
    return -NQ_ENOMEM;
  s->request = request;
  if (len)
    NQMemcpy(request, data, len);

  IP_OPTION_INFORMATION opt;
  NQMemset(&opt, 0, sizeof(opt));
  opt.Ttl = (UCHAR)(s->ttl > 0 ? s->ttl : 128);
  ResetEvent(s->event);
  DWORD rc;
  if (s->v6) {
    struct sockaddr_in6 src, dst;
    NQMemset(&src, 0, sizeof(src));
    NQMemset(&dst, 0, sizeof(dst));
    src.sin6_family = dst.sin6_family = AF_INET6;
    NQMemcpy(&dst.sin6_addr, s->to.ip6ep.address.data, 16);
    rc = Icmp6SendEcho2(s->icmp, s->event, NULL, NULL, &src, &dst, request, (WORD)len, &opt, s->reply, s->replySize,
                        ICMP_TIMEOUT_MS);
  } else {
    IPAddr dst;
    NQMemcpy(&dst, s->to.ip4ep.address.data, 4);
    rc = IcmpSendEcho2(s->icmp, s->event, NULL, NULL, dst, request, (WORD)len, &opt, s->reply, s->replySize,
                       ICMP_TIMEOUT_MS);
  }
  if (rc == 0 && GetLastError() != ERROR_IO_PENDING)
    return -(int)GetLastError();
  s->pending = true;
  s->id = id;
  s->seq = seq;
  s->len = len;
  return 0;
}

/* A failed request's status as the ICMP message that would say so */
static bool status_to_icmp(bool v6, ULONG status, NQIcmpReply* r)
{
  static const struct {
    ULONG status;
    int type4, code4, type6, code6;
  } map[] = {
    { IP_DEST_NET_UNREACHABLE, 3, 0, 1, 0 },
    { IP_DEST_HOST_UNREACHABLE, 3, 1, 1, 3 },
    { IP_DEST_PROT_UNREACHABLE, 3, 2, 1, 4 },
    { IP_DEST_PORT_UNREACHABLE, 3, 3, 1, 4 },
    { IP_PACKET_TOO_BIG, 3, 4, 2, 0 },
    { IP_TTL_EXPIRED_TRANSIT, 11, 0, 3, 0 },
    { IP_TTL_EXPIRED_REASSEM, 11, 1, 3, 1 },
  };
  for (size_t i = 0; i < NQ_ARRAY_LENGTH(map); i++) {
    if (map[i].status == status) {
      r->type = v6 ? map[i].type6 : map[i].type4;
      r->code = v6 ? map[i].code6 : map[i].code4;
      return true;
    }
  }
  return false;
}

int NQIcmpSocket_receive(NQIcmpSocket* s, NQIcmpReply* r)
{
  if (!s->pending || WaitForSingleObject(s->event, 0) != WAIT_OBJECT_0)
    return 0;
  ResetEvent(s->event);
  s->pending = false;

  NQMemset(r, 0, sizeof(*r));
  r->ttl = -1;
  r->id = s->id;
  r->seq = s->seq;
  r->length = s->len + 8;
  ULONG status;
  if (s->v6) {
    if (Icmp6ParseReplies(s->reply, s->replySize) == 0)
      return 0;
    const NQIcmp6EchoReply* rep = (const NQIcmp6EchoReply*)s->reply;
    status = rep->Status;
    NQEndPoint_init6(&r->from, (const uint8_t*)rep->Address.sin6_addr, 0);
  } else {
    if (IcmpParseReplies(s->reply, s->replySize) == 0)
      return 0;
    PICMP_ECHO_REPLY rep = (PICMP_ECHO_REPLY)s->reply;
    status = rep->Status;
    r->ttl = rep->Options.Ttl;
    r->length = rep->DataSize + 8;
    NQEndPoint_init4(&r->from, (const uint8_t*)&rep->Address, 0);
  }
  if (status == IP_SUCCESS) {
    r->kind = NQ_ICMP_ECHO_REPLY;
    r->type = s->v6 ? ICMP6_ECHO_REPLY_ : ICMP_ECHO_REPLY_;
    return 1;
  }
  if (status_to_icmp(s->v6, status, r)) {
    r->kind = NQ_ICMP_ERROR;
    return 1;
  }
  return 0; /* timed out, or nothing to say */
}

#else

int NQIcmpSocket_open(int family, NQIcmpSocket** result)
{
  NQ_UNUSED_PARAM(family);
  *result = NULL;
  return -NQ_ENOTSUPP;
}

void NQIcmpSocket_close(NQIcmpSocket* s)
{
  NQ_UNUSED_PARAM(s);
}

intptr_t NQIcmpSocket_handle(NQIcmpSocket* s)
{
  NQ_UNUSED_PARAM(s);
  return -1;
}

bool NQIcmpSocket_isRaw(NQIcmpSocket* s)
{
  NQ_UNUSED_PARAM(s);
  return false;
}

bool NQIcmpSocket_ownsReplies(NQIcmpSocket* s)
{
  NQ_UNUSED_PARAM(s);
  return false;
}

int NQIcmpSocket_setTtl(NQIcmpSocket* s, int ttl)
{
  NQ_UNUSED_PARAM(s);
  NQ_UNUSED_PARAM(ttl);
  return -NQ_ENOTSUPP;
}

int NQIcmpSocket_connect(NQIcmpSocket* s, const NQEndPoint* to)
{
  NQ_UNUSED_PARAM(s);
  NQ_UNUSED_PARAM(to);
  return -NQ_ENOTSUPP;
}

int NQIcmpSocket_sendEcho(NQIcmpSocket* s, uint16_t id, uint16_t seq, const void* data, size_t len)
{
  NQ_UNUSED_PARAM(s);
  NQ_UNUSED_PARAM(id);
  NQ_UNUSED_PARAM(seq);
  NQ_UNUSED_PARAM(data);
  NQ_UNUSED_PARAM(len);
  return -NQ_ENOTSUPP;
}

int NQIcmpSocket_receive(NQIcmpSocket* s, NQIcmpReply* r)
{
  NQ_UNUSED_PARAM(s);
  NQ_UNUSED_PARAM(r);
  return -NQ_ENOTSUPP;
}

#endif
