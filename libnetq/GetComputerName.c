/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/GetComputerName.h"

#include <libnetq/ErrorCode.h>
#include <libnetq/string/String.h>

#ifdef NQ_OS_KERNEL
#include <linux/utsname.h>
#define NQ_COMPUTER_NAME_MAX (__NEW_UTS_LEN + 1)
#endif

#ifdef NQ_OS_WINDOWS
#include <windows.h>
#include <libnetq/Malloc.h>
#include <libnetq/UTF.h>
#define NQ_COMPUTER_NAME_MAX (MAX_COMPUTERNAME_LENGTH + 1)
#endif

#ifdef NQ_OS_UNIX
#include <unistd.h>
#ifdef NQ_OS_ANDROID
#include <sys/utsname.h>
#if __ANDROID_API__ < 23 /* Bionic declares sethostname() only for API level 23+ */
#include <sys/syscall.h>
#endif
#endif
#define NQ_COMPUTER_NAME_MAX (HOST_NAME_MAX + 1)
#endif

/* Empties the buffer and returns the negated error code. */
static int fail(char* buffer, size_t n, NQErrorCode ec)
{
  if (n != 0)
    *buffer = '\0';
  return -ec;
}

int NQGetComputerName(char* buffer, size_t n)
{
  if (n == 0)
    return -NQ_ENAMETOOLONG;

#if defined(NQ_OS_KERNEL)
  struct new_utsname* u = init_utsname();
  size_t res = NQStrlen(u->nodename);
  if (res >= n)
    return fail(buffer, n, NQ_ENAMETOOLONG);
  NQMemcpy(buffer, u->nodename, res + 1);
  return (int)res;

#elif defined(NQ_OS_WINDOWS)
  WCHAR wbuf[NQ_COMPUTER_NAME_MAX];
  DWORD size = NQ_ARRAY_LENGTH(wbuf); /* in: capacity, out: length without the terminator */
  NQUnicodeInfo info;

  if (!GetComputerNameW(wbuf, &size))
    return fail(buffer, n, NQGetLastError());

  const uint16_t* start = (const uint16_t*)wbuf;
  if (!NQCalculateUTF16Info(start, start + size, &info))
    return fail(buffer, n, NQ_EINVAL);
  if (info.utf8Size >= n)
    return fail(buffer, n, NQ_ENAMETOOLONG);

  NQConvertUTF16ToUTF8(start, start + size, (uint8_t*)buffer, (uint8_t*)buffer + n, NULL);
  buffer[info.utf8Size] = '\0';
  return (int)info.utf8Size;

#elif defined(NQ_OS_UNIX)
  if (gethostname(buffer, n) != 0)
    return fail(buffer, n, NQGetLastError());
  /* POSIX allows a silently truncated, unterminated name */
  if (NQMemchr(buffer, '\0', n) == NULL)
    return fail(buffer, n, NQ_ENAMETOOLONG);
  return (int)NQStrlen(buffer);

#else
  return fail(buffer, n, NQ_ENOTSUPP);

#endif
}

int NQSetComputerName(const char* name)
{
#if defined(NQ_OS_WINDOWS)
  NQUnicodeInfo info;
  if (!NQConvertUTF8ToUTF16((const uint8_t*)name, NULL, NULL, NULL, &info))
    return -NQ_EINVAL;
  WCHAR* wide = (WCHAR*)NQMalloc((info.utf16Size + 1) * sizeof(WCHAR));
  if (wide == NULL)
    return -NQ_ENOMEM;
  NQConvertUTF8ToUTF16((const uint8_t*)name, (const uint8_t*)(name + info.utf8Size), (uint16_t*)wide,
                       (uint16_t*)wide + info.utf16Size, NULL);
  wide[info.utf16Size] = 0;

  BOOL done = SetComputerNameExW(ComputerNamePhysicalDnsHostname, wide);
  DWORD error = done ? 0 : GetLastError();
  NQFree(wide);
  if (error == ERROR_INVALID_COMPUTERNAME)
    return -NQ_EINVAL;
  return -(int)error;

#elif defined(NQ_OS_ANDROID) && __ANDROID_API__ < 23
  return syscall(__NR_sethostname, name, NQStrlen(name)) == 0 ? 0 : -NQGetLastError();

#elif defined(NQ_OS_UNIX)
  return sethostname(name, NQStrlen(name)) == 0 ? 0 : -NQGetLastError();

#else
  NQ_UNUSED_PARAM(name);
  return -NQ_ENOTSUPP;
#endif
}

int NQGetNisDomainName(char* buffer, size_t n)
{
  if (n == 0)
    return -NQ_ENAMETOOLONG;
  buffer[0] = '\0';

#if defined(NQ_OS_ANDROID)
  /* getdomainname needs a later API level than the one built for */
  struct utsname u;
  if (uname(&u) != 0)
    return -NQGetLastError();
  size_t len = NQStrlen(u.domainname);
  if (len >= n)
    return -NQ_ENAMETOOLONG;
  NQMemcpy(buffer, u.domainname, len + 1);
  return (int)len;

#elif defined(NQ_OS_UNIX)
  if (getdomainname(buffer, n) != 0)
    return fail(buffer, n, NQGetLastError());
  /* as with gethostname, the name may be cut without its terminator */
  if (NQMemchr(buffer, '\0', n) == NULL)
    return fail(buffer, n, NQ_ENAMETOOLONG);
  return (int)NQStrlen(buffer);

#else
  return -NQ_ENOTSUPP;
#endif
}

int NQSetNisDomainName(const char* name)
{
#if defined(NQ_OS_UNIX) && !defined(NQ_OS_ANDROID)
  return setdomainname(name, NQStrlen(name)) == 0 ? 0 : -NQGetLastError();
#else
  NQ_UNUSED_PARAM(name);
  return -NQ_ENOTSUPP;
#endif
}
