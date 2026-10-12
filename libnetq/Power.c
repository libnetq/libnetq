/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/Power.h"

#include <libnetq/ErrorCode.h>
#include <libnetq/string/Sprintf.h>
#include <libnetq/string/String.h>

#if defined(NQ_OS_WINDOWS)
# include <windows.h>
#elif defined(NQ_OS_UNIX)
# include <unistd.h>
# include <sys/reboot.h>
# if defined(NQ_OS_LINUX)
#  include <fcntl.h>
#  include <stdlib.h>
#  include <linux/reboot.h>
#  include <sys/socket.h>
#  include <sys/stat.h>
#  include <sys/syscall.h>
#  include <sys/time.h>
#  include <sys/un.h>
#  include <libnetq/Malloc.h>
# endif
# if defined(NQ_OS_DARWIN)
#  include <TargetConditionals.h>
#  if TARGET_OS_OSX
#   include <CoreServices/CoreServices.h>
#   define NQ_POWER_LOGINWINDOW 1
#  endif
# endif
#endif

#if defined(NQ_OS_LINUX)

# define REBOOT_PARAM_FILE "/run/systemd/reboot-param"
# define REBOOT_PARAM_MAX 256

/* Reads the parameter NQSetRebootParameter left, without the line end;
 * returns its length, 0 when there is none */
static size_t read_reboot_parameter(char* buf, size_t n)
{
  int fd = open(REBOOT_PARAM_FILE, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    return 0;
  ssize_t len = read(fd, buf, n - 1);
  close(fd);
  if (len < 0)
    len = 0;
  while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
    len--;
  buf[len] = '\0';
  return (size_t)len;
}

#endif

#if defined(NQ_OS_LINUX) || defined(NQ_OS_DARWIN)

/* The reboot system call: what NQ_POWER_FORCE does */
static int power_now(NQPowerAction action, unsigned flags)
{
# if defined(NQ_OS_LINUX)
  static const int commands[] = { RB_POWER_OFF, RB_AUTOBOOT, RB_HALT_SYSTEM };
  if (!(flags & NQ_POWER_NO_SYNC))
    sync();
  if (action == NQ_POWER_REBOOT) {
    /* with an argument, as systemd's reboot_with_parameter; without one
     * if the kernel will not take it */
    char param[REBOOT_PARAM_MAX];
    if (read_reboot_parameter(param, sizeof(param)) > 0)
      syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART2, param);
  }
  reboot(commands[action]);
# else
  /* the Darwin kernel syncs itself unless told not to; halting a Mac
   * switches it off */
  int howto = action == NQ_POWER_REBOOT ? RB_AUTOBOOT : RB_HALT;
  if (flags & NQ_POWER_NO_SYNC)
    howto |= RB_NOSYNC;
  reboot(howto);
# endif
  return -NQGetLastError();
}

#endif

/* --- Linux: systemd-logind and systemd over the D-Bus system bus ---------- */

#if defined(NQ_OS_LINUX)

/* How long the bus may take to answer, as sd-bus's default */
# define DBUS_TIMEOUT_SEC 25
# define DBUS_MAX_MESSAGE (1u << 20)

# define DBUS_METHOD_CALL   1
# define DBUS_METHOD_RETURN 2
# define DBUS_ERROR         3

/* Lets polkit ask for a password through the session's agent */
# define DBUS_FLAG_ALLOW_INTERACTIVE_AUTHORIZATION 0x4

# define DBUS_FIELD_PATH         1
# define DBUS_FIELD_INTERFACE    2
# define DBUS_FIELD_MEMBER       3
# define DBUS_FIELD_ERROR_NAME   4
# define DBUS_FIELD_REPLY_SERIAL 5
# define DBUS_FIELD_DESTINATION  6
# define DBUS_FIELD_SIGNATURE    8

/* A message being marshalled, in native byte order */
typedef struct DBusBuffer {
  uint8_t data[512];
  size_t len;
  bool overflow;
} DBusBuffer;

static char dbus_native_endian(void)
{
  const uint16_t one = 1;
  return *(const uint8_t*)&one ? 'l' : 'B';
}

static void dbus_put(DBusBuffer* b, const void* p, size_t n)
{
  if (b->overflow || n > sizeof(b->data) - b->len) {
    b->overflow = true;
    return;
  }
  NQMemcpy(b->data + b->len, p, n);
  b->len += n;
}

static void dbus_put_byte(DBusBuffer* b, uint8_t v)
{
  dbus_put(b, &v, 1);
}

static void dbus_align(DBusBuffer* b, size_t a)
{
  while (b->len % a != 0 && !b->overflow)
    dbus_put_byte(b, 0);
}

static void dbus_put_u32(DBusBuffer* b, uint32_t v)
{
  dbus_align(b, 4);
  dbus_put(b, &v, 4);
}

static void dbus_put_string(DBusBuffer* b, const char* s)
{
  size_t n = NQStrlen(s);
  dbus_put_u32(b, (uint32_t)n);
  dbus_put(b, s, n + 1);
}

static void dbus_put_signature(DBusBuffer* b, const char* s)
{
  size_t n = NQStrlen(s);
  dbus_put_byte(b, (uint8_t)n);
  dbus_put(b, s, n + 1);
}

/* A header field: a (yv) struct whose variant holds a string, an object
 * path or a signature */
static void dbus_put_field(DBusBuffer* b, uint8_t code, char type, const char* value)
{
  const char signature[2] = { type, '\0' };
  dbus_align(b, 8);
  dbus_put_byte(b, code);
  dbus_put_signature(b, signature);
  if (type == 'g')
    dbus_put_signature(b, value);
  else
    dbus_put_string(b, value);
}

static int dbus_send_all(int fd, const void* data, size_t n)
{
  const uint8_t* p = data;
  while (n > 0) {
    ssize_t r = send(fd, p, n, MSG_NOSIGNAL);
    if (r < 0) {
      if (errno == EINTR)
        continue;
      return errno == EAGAIN || errno == EWOULDBLOCK ? -NQ_ETIMEDOUT : -errno;
    }
    p += r;
    n -= (size_t)r;
  }
  return 0;
}

static int dbus_recv_all(int fd, void* data, size_t n)
{
  uint8_t* p = data;
  while (n > 0) {
    ssize_t r = recv(fd, p, n, 0);
    if (r < 0) {
      if (errno == EINTR)
        continue;
      return errno == EAGAIN || errno == EWOULDBLOCK ? -NQ_ETIMEDOUT : -errno;
    }
    if (r == 0)
      return -NQ_EIO;
    p += r;
    n -= (size_t)r;
  }
  return 0;
}

/* Sends a method call whose body, already marshalled from offset 0, has
 * the signature `signature` (NULL for none) */
static int dbus_call(int fd, uint32_t serial, uint8_t flags, const char* destination,
                     const char* path, const char* interface, const char* member,
                     const char* signature, const DBusBuffer* body)
{
  DBusBuffer m;
  m.len = 0;
  m.overflow = false;

  dbus_put_byte(&m, (uint8_t)dbus_native_endian());
  dbus_put_byte(&m, DBUS_METHOD_CALL);
  dbus_put_byte(&m, flags);
  dbus_put_byte(&m, 1); /* protocol version */
  dbus_put_u32(&m, body ? (uint32_t)body->len : 0);
  dbus_put_u32(&m, serial);
  dbus_put_u32(&m, 0); /* length of the header fields, set below */
  dbus_put_field(&m, DBUS_FIELD_PATH, 'o', path);
  dbus_put_field(&m, DBUS_FIELD_INTERFACE, 's', interface);
  dbus_put_field(&m, DBUS_FIELD_MEMBER, 's', member);
  dbus_put_field(&m, DBUS_FIELD_DESTINATION, 's', destination);
  if (signature)
    dbus_put_field(&m, DBUS_FIELD_SIGNATURE, 'g', signature);
  uint32_t fields = (uint32_t)(m.len - 16);
  if (!m.overflow)
    NQMemcpy(m.data + 12, &fields, 4);
  dbus_align(&m, 8); /* the body starts on an 8-byte boundary */
  if (body)
    dbus_put(&m, body->data, body->len);
  if (m.overflow)
    return -NQ_EINVAL;
  return dbus_send_all(fd, m.data, m.len);
}

/* A received message being read */
typedef struct DBusReader {
  const uint8_t* data;
  size_t len, pos;
  bool swap, ok;
} DBusReader;

static uint32_t dbus_get_u32_at(const uint8_t* p, bool swap)
{
  uint32_t v;
  NQMemcpy(&v, p, 4);
  if (swap)
    v = (v >> 24) | ((v >> 8) & 0xff00u) | ((v << 8) & 0xff0000u) | (v << 24);
  return v;
}

static void dbus_skip_to(DBusReader* r, size_t align)
{
  r->pos = (r->pos + align - 1) / align * align;
  if (r->pos > r->len)
    r->ok = false;
}

static uint8_t dbus_get_byte(DBusReader* r)
{
  if (!r->ok || r->pos >= r->len) {
    r->ok = false;
    return 0;
  }
  return r->data[r->pos++];
}

static uint32_t dbus_get_u32(DBusReader* r)
{
  dbus_skip_to(r, 4);
  if (!r->ok || r->len - r->pos < 4) {
    r->ok = false;
    return 0;
  }
  uint32_t v = dbus_get_u32_at(r->data + r->pos, r->swap);
  r->pos += 4;
  return v;
}

/* Takes `n` bytes and the NUL after them; returns them, or NULL */
static const char* dbus_get_chars(DBusReader* r, size_t n)
{
  if (!r->ok || r->len - r->pos <= n || r->data[r->pos + n] != '\0') {
    r->ok = false;
    return NULL;
  }
  const char* s = (const char*)r->data + r->pos;
  r->pos += n + 1;
  return s;
}

/* What an error reply means to NQPowerControl: NQ_ENOSYS for a service
 * or method that is not there, so that the next way is tried */
static int dbus_error_code(const char* name)
{
  static const char* const missing[] = {
    "org.freedesktop.DBus.Error.ServiceUnknown",
    "org.freedesktop.DBus.Error.NameHasNoOwner",
    "org.freedesktop.DBus.Error.UnknownMethod",
    "org.freedesktop.DBus.Error.UnknownObject",
    "org.freedesktop.DBus.Error.UnknownInterface",
  };
  for (size_t i = 0; i < NQ_ARRAY_LENGTH(missing); i++) {
    if (NQStrcmp(name, missing[i]) == 0)
      return -NQ_ENOSYS;
  }
  if (NQStrcmp(name, "org.freedesktop.DBus.Error.AccessDenied") == 0
      || NQStrcmp(name, "org.freedesktop.DBus.Error.InteractiveAuthorizationRequired") == 0)
    return -NQ_EACCES;
  if (NQStrcmp(name, "org.freedesktop.login1.OperationInProgress") == 0)
    return -NQ_EINPROGRESS;
  return -NQ_EIO;
}

/* Reads messages, skipping signals, until the reply to `serial`. Returns
 * 0 for a method return, or the error it stands for. */
static int dbus_read_reply(int fd, uint32_t serial)
{
  for (;;) {
    uint8_t head[16];
    int ret = dbus_recv_all(fd, head, sizeof(head));
    if (ret != 0)
      return ret;

    bool swap = head[0] != (uint8_t)dbus_native_endian();
    uint32_t body_len = dbus_get_u32_at(head + 4, swap);
    uint32_t fields_len = dbus_get_u32_at(head + 12, swap);
    if ((head[0] != 'l' && head[0] != 'B') || body_len > DBUS_MAX_MESSAGE
        || fields_len > DBUS_MAX_MESSAGE)
      return -NQ_EIO;

    size_t fields_end = 16 + (size_t)fields_len;
    size_t total = (fields_end + 7) / 8 * 8 + body_len;
    uint8_t* msg = NQMalloc(total);
    if (!msg)
      return -NQ_ENOMEM;
    NQMemcpy(msg, head, sizeof(head));
    ret = dbus_recv_all(fd, msg + sizeof(head), total - sizeof(head));
    if (ret != 0) {
      NQFree(msg);
      return ret;
    }

    DBusReader r = { msg, fields_end, 16, swap, true };
    uint32_t reply_serial = 0;
    const char* error_name = "";
    while (r.ok && r.pos < r.len) {
      dbus_skip_to(&r, 8);
      if (r.pos >= r.len)
        break;
      uint8_t code = dbus_get_byte(&r);
      const char* type = dbus_get_chars(&r, dbus_get_byte(&r));
      if (!type)
        break;
      if (NQStrcmp(type, "u") == 0) {
        uint32_t v = dbus_get_u32(&r);
        if (code == DBUS_FIELD_REPLY_SERIAL)
          reply_serial = v;
      } else if (NQStrcmp(type, "s") == 0 || NQStrcmp(type, "o") == 0) {
        const char* s = dbus_get_chars(&r, dbus_get_u32(&r));
        if (s && code == DBUS_FIELD_ERROR_NAME)
          error_name = s;
      } else if (NQStrcmp(type, "g") == 0) {
        dbus_get_chars(&r, dbus_get_byte(&r));
      } else {
        r.ok = false; /* no other types are used in header fields */
      }
    }

    uint8_t type = head[1];
    if (!r.ok) {
      ret = -NQ_EIO;
    } else if ((type == DBUS_METHOD_RETURN || type == DBUS_ERROR) && reply_serial == serial) {
      ret = type == DBUS_METHOD_RETURN ? 0 : dbus_error_code(error_name);
    } else {
      ret = 1; /* not ours: a signal, such as NameAcquired */
    }
    NQFree(msg);
    if (ret <= 0)
      return ret;
  }
}

/* Unescapes the %XX of a D-Bus address value up to `end` */
static bool dbus_unescape(const char* s, const char* end, char* out, size_t n)
{
  size_t len = 0;
  while (s < end) {
    char c = *s++;
    if (c == '%') {
      unsigned v = 0;
      for (int i = 0; i < 2; i++, s++) {
        if (s >= end)
          return false;
        char h = *s;
        v <<= 4;
        if (h >= '0' && h <= '9')
          v |= (unsigned)(h - '0');
        else if (h >= 'a' && h <= 'f')
          v |= (unsigned)(h - 'a' + 10);
        else if (h >= 'A' && h <= 'F')
          v |= (unsigned)(h - 'A' + 10);
        else
          return false;
      }
      c = (char)v;
    }
    if (len + 1 >= n)
      return false;
    out[len++] = c;
  }
  out[len] = '\0';
  return true;
}

/* Finds the socket of the first "unix:" address in $DBUS_SYSTEM_BUS_ADDRESS,
 * or the standard one. Sets *abstract for an abstract socket name. */
static bool dbus_system_bus_path(char* path, size_t n, bool* abstract)
{
  const char* address = getenv("DBUS_SYSTEM_BUS_ADDRESS");
  *abstract = false;
  if (!address || !*address)
    address = "unix:path=/var/run/dbus/system_bus_socket";

  for (const char* entry = address; *entry;) {
    const char* entry_end = NQStrchr(entry, ';');
    if (!entry_end)
      entry_end = entry + NQStrlen(entry);
    if (NQStrncmp(entry, "unix:", 5) == 0) {
      for (const char* key = entry + 5; key < entry_end;) {
        const char* key_end = NQMemchr(key, ',', (size_t)(entry_end - key));
        if (!key_end)
          key_end = entry_end;
        if (NQStrncmp(key, "path=", 5) == 0 && key + 5 <= key_end)
          return dbus_unescape(key + 5, key_end, path, n);
        if (NQStrncmp(key, "abstract=", 9) == 0 && key + 9 <= key_end) {
          *abstract = true;
          return dbus_unescape(key + 9, key_end, path, n);
        }
        key = key_end < entry_end ? key_end + 1 : entry_end;
      }
    }
    entry = *entry_end ? entry_end + 1 : entry_end;
  }
  return false;
}

/* Connects to the system bus, authenticates as the effective user and
 * says Hello. Returns the socket, or a negated error code. */
static int dbus_open(void)
{
  struct sockaddr_un sa;
  char path[sizeof(sa.sun_path)];
  bool abstract;
  if (!dbus_system_bus_path(path, sizeof(path) - 1, &abstract))
    return -NQ_ENOENT;

  NQMemset(&sa, 0, sizeof(sa));
  sa.sun_family = AF_UNIX;
  size_t path_len = NQStrlen(path);
  NQMemcpy(sa.sun_path + (abstract ? 1 : 0), path, path_len);
  socklen_t sa_len = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + path_len
                                 + (abstract ? 1 : 0));

  int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (fd < 0)
    return -errno;
  struct timeval timeout = { DBUS_TIMEOUT_SEC, 0 };
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
  if (connect(fd, (const struct sockaddr*)&sa, sa_len) != 0) {
    int ret = -errno;
    close(fd);
    return ret;
  }

  /* SASL EXTERNAL: the user ID in decimal, hex-encoded */
  char uid[24], auth[96];
  static const char hex[] = "0123456789abcdef";
  int uid_len = NQSnprintf(uid, sizeof(uid), "%lu", (unsigned long)geteuid());
  size_t len = 0;
  const char* begin = "AUTH EXTERNAL ";
  auth[len++] = '\0'; /* the credentials byte */
  NQMemcpy(auth + len, begin, NQStrlen(begin));
  len += NQStrlen(begin);
  for (int i = 0; i < uid_len; i++) {
    auth[len++] = hex[(uint8_t)uid[i] >> 4];
    auth[len++] = hex[(uint8_t)uid[i] & 0xf];
  }
  auth[len++] = '\r';
  auth[len++] = '\n';
  int ret = dbus_send_all(fd, auth, len);

  /* the server says nothing more before BEGIN, so the line is all there is */
  char line[256];
  len = 0;
  while (ret == 0) {
    if (len + 1 >= sizeof(line)) {
      ret = -NQ_EIO;
      break;
    }
    ssize_t r = recv(fd, line + len, sizeof(line) - 1 - len, 0);
    if (r < 0 && errno == EINTR)
      continue;
    if (r <= 0) {
      ret = r == 0 ? -NQ_EIO : (errno == EAGAIN || errno == EWOULDBLOCK ? -NQ_ETIMEDOUT : -errno);
      break;
    }
    len += (size_t)r;
    line[len] = '\0';
    if (NQStrstr(line, "\r\n"))
      break;
  }
  if (ret == 0 && NQStrncmp(line, "OK ", 3) != 0)
    ret = -NQ_EACCES;
  if (ret == 0)
    ret = dbus_send_all(fd, "BEGIN\r\n", 7);
  if (ret == 0)
    ret = dbus_call(fd, 1, 0, "org.freedesktop.DBus", "/org/freedesktop/DBus",
                    "org.freedesktop.DBus", "Hello", NULL, NULL);
  if (ret == 0)
    ret = dbus_read_reply(fd, 1);
  if (ret != 0) {
    close(fd);
    return ret;
  }
  return fd;
}

/* What systemctl does first: logind's PowerOff, Reboot or Halt, which
 * checks inhibitors and polkit before starting the shutdown */
static int ask_logind(int fd, NQPowerAction action)
{
  static const char* const methods[] = { "PowerOff", "Reboot", "Halt" };
  DBusBuffer body;
  body.len = 0;
  body.overflow = false;
  dbus_put_u32(&body, 1); /* interactive: polkit may ask for a password */
  int ret = dbus_call(fd, 2, DBUS_FLAG_ALLOW_INTERACTIVE_AUTHORIZATION, "org.freedesktop.login1",
                      "/org/freedesktop/login1", "org.freedesktop.login1.Manager",
                      methods[action], "b", &body);
  return ret != 0 ? ret : dbus_read_reply(fd, 2);
}

/* systemctl's fallback without logind: starting the shutdown target */
static int ask_systemd(int fd, NQPowerAction action)
{
  static const char* const units[] = { "poweroff.target", "reboot.target", "halt.target" };
  DBusBuffer body;
  body.len = 0;
  body.overflow = false;
  dbus_put_string(&body, units[action]);
  dbus_put_string(&body, "replace-irreversibly");
  int ret = dbus_call(fd, 3, DBUS_FLAG_ALLOW_INTERACTIVE_AUTHORIZATION, "org.freedesktop.systemd1",
                      "/org/freedesktop/systemd1", "org.freedesktop.systemd1.Manager", "StartUnit",
                      "ss", &body);
  return ret != 0 ? ret : dbus_read_reply(fd, 3);
}

static int power_linux(NQPowerAction action, unsigned flags)
{
  if (!(flags & NQ_POWER_FORCE)) {
    int fd = dbus_open();
    if (fd >= 0) {
      int ret = ask_logind(fd, action);
      if (ret == -NQ_ENOSYS)
        ret = ask_systemd(fd, action);
      close(fd);
      if (ret != -NQ_ENOSYS)
        return ret;
    }
    /* no bus, or nothing on it to ask: only root may go on */
    if (geteuid() != 0)
      return -NQ_EPERM;
  }
  return power_now(action, flags);
}

#endif /* NQ_OS_LINUX */

/* --- macOS: loginwindow --------------------------------------------------- */

#if defined(NQ_POWER_LOGINWINDOW)

/* The Apple event the Apple menu sends: loginwindow asks applications to
 * quit, then shuts down or restarts */
static int ask_loginwindow(NQPowerAction action)
{
  static const char bundle[] = "com.apple.loginwindow";
  AEAddressDesc target;
  AppleEvent event = { typeNull, NULL }, reply = { typeNull, NULL };
  AEEventID id = action == NQ_POWER_REBOOT ? kAERestart : kAEShutDown;

  OSStatus st = AECreateDesc(typeApplicationBundleID, bundle, sizeof(bundle) - 1, &target);
  if (st == noErr) {
    st = AECreateAppleEvent(kCoreEventClass, id, &target, kAutoGenerateReturnID,
                            kAnyTransactionID, &event);
    AEDisposeDesc(&target);
  }
  if (st == noErr)
    st = AESendMessage(&event, &reply, kAENormalPriority, kAEDefaultTimeout);
  AEDisposeDesc(&event);
  AEDisposeDesc(&reply);

  switch (st) {
  case noErr:
    return 0;
  case errAEEventNotPermitted:
  case userCanceledErr:
    return -NQ_EACCES;
  case errAETimeout:
    return -NQ_ETIMEDOUT;
  default:
    return -NQ_EIO;
  }
}

#endif

/* --- Windows -------------------------------------------------------------- */

#if defined(NQ_OS_WINDOWS)

/* ExitWindowsEx needs SeShutdownPrivilege enabled, which users hold but
 * do not have enabled */
static int enable_shutdown_privilege(void)
{
  HANDLE token;
  TOKEN_PRIVILEGES tp;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
    return -(int)GetLastError();
  if (!LookupPrivilegeValueW(NULL, L"SeShutdownPrivilege", &tp.Privileges[0].Luid)) {
    DWORD ec = GetLastError();
    CloseHandle(token);
    return -(int)ec;
  }
  tp.PrivilegeCount = 1;
  tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  /* succeeds even when the privilege is not held */
  BOOL ok = AdjustTokenPrivileges(token, FALSE, &tp, 0, NULL, NULL);
  DWORD ec = GetLastError();
  CloseHandle(token);
  if (!ok)
    return -(int)ec;
  if (ec == ERROR_NOT_ALL_ASSIGNED)
    return -NQ_EPERM;
  return 0;
}

static int power_windows(NQPowerAction action, unsigned flags)
{
  static const UINT modes[] = { EWX_POWEROFF, EWX_REBOOT, EWX_SHUTDOWN };
  int ret = enable_shutdown_privilege();
  if (ret != 0)
    return ret;
  UINT mode = modes[action];
  if (flags & NQ_POWER_FORCE)
    mode |= EWX_FORCE;
  if (!ExitWindowsEx(mode, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER
                             | SHTDN_REASON_FLAG_PLANNED))
    return -(int)GetLastError();
  return 0;
}

#endif

int NQSetRebootParameter(const char* parameter)
{
#if defined(NQ_OS_LINUX)
  if (!parameter || !*parameter) {
    if (unlink(REBOOT_PARAM_FILE) == 0 || errno == ENOENT)
      return 0;
    return -errno;
  }
  size_t len = NQStrlen(parameter);
  if (len >= REBOOT_PARAM_MAX)
    return -NQ_ENAMETOOLONG;

  /* written aside and renamed into place, as systemd does */
  char tmp[] = REBOOT_PARAM_FILE ".XXXXXX";
#if defined(HAVE_MKOSTEMP)
  int fd = mkostemp(tmp, O_CLOEXEC);
  if (fd < 0)
    return -errno;
#else
  /* Android before API 23 */
  int fd = mkstemp(tmp);
  if (fd < 0)
    return -errno;
  fcntl(fd, F_SETFD, FD_CLOEXEC);
#endif
  int ret = fchmod(fd, 0644) == 0 ? 0 : -errno;
  for (size_t done = 0; ret == 0 && done < len;) {
    ssize_t n = write(fd, parameter + done, len - done);
    if (n < 0 && errno != EINTR)
      ret = -errno;
    else if (n > 0)
      done += (size_t)n;
  }
  if (ret == 0 && write(fd, "\n", 1) != 1)
    ret = -errno;
  if (close(fd) != 0 && ret == 0)
    ret = -errno;
  if (ret == 0 && rename(tmp, REBOOT_PARAM_FILE) != 0)
    ret = -errno;
  if (ret != 0)
    unlink(tmp);
  return ret;

#else
  return parameter && *parameter ? -NQ_ENOTSUPP : 0;

#endif
}

int NQPowerControl(NQPowerAction action, unsigned flags)
{
  if ((unsigned)action > NQ_POWER_HALT)
    return -NQ_EINVAL;

#if defined(NQ_OS_WINDOWS)
  return power_windows(action, flags);

#elif defined(NQ_OS_LINUX)
  return power_linux(action, flags);

#elif defined(NQ_OS_DARWIN)
# if defined(NQ_POWER_LOGINWINDOW)
  if (!(flags & NQ_POWER_FORCE))
    return ask_loginwindow(action);
# endif
  return power_now(action, flags);

#else
  NQ_UNUSED_PARAM(flags);
  return -NQ_ENOTSUPP;

#endif
}
