/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/GetUserName.h"

#include <libnetq/ErrorCode.h>
#include <libnetq/string/String.h>

#ifdef NQ_OS_WINDOWS
#include <windows.h>
#include <libnetq/UTF.h>
#include <libnetq/fs/Path.h>
#endif

#ifdef NQ_OS_UNIX
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include <libnetq/Malloc.h>
#endif

/* Empties the buffer and returns the negated error code. */
static int fail(char* buffer, size_t n, NQErrorCode ec)
{
  if (n != 0)
    *buffer = '\0';
  return -ec;
}

/* Copies `str` with its terminator if it fits. */
static int copy_value(char* buffer, size_t n, const char* str)
{
  size_t len = (str != NULL) ? NQStrlen(str) : 0;
  if (len == 0)
    return fail(buffer, n, NQ_ENOENT);
  if (len >= n)
    return fail(buffer, n, NQ_ENAMETOOLONG);
  NQMemcpy(buffer, str, len + 1);
  return (int)len;
}

#ifdef NQ_OS_WINDOWS

/* GetUserNameW reports the user of the access token, which serves as both
 * the login and the effective user. */
static int token_user_name(char* buffer, size_t n)
{
  WCHAR wbuf[256 + 1]; /* UNLEN + 1 */
  DWORD size = NQ_ARRAY_LENGTH(wbuf); /* in: capacity, out: length with the terminator */
  NQUnicodeInfo info;

  if (!GetUserNameW(wbuf, &size))
    return fail(buffer, n, NQGetLastError());
  if (size <= 1)
    return fail(buffer, n, NQ_ENOENT);

  const uint16_t* start = (const uint16_t*)wbuf;
  const uint16_t* end = start + size - 1;
  if (!NQCalculateUTF16Info(start, end, &info))
    return fail(buffer, n, NQ_EINVAL);
  if (info.utf8Size >= n)
    return fail(buffer, n, NQ_ENAMETOOLONG);

  NQConvertUTF16ToUTF8(start, end, (uint8_t*)buffer, (uint8_t*)buffer + n, NULL);
  buffer[info.utf8Size] = '\0';
  return (int)info.utf8Size;
}

#endif /* NQ_OS_WINDOWS */

#ifdef NQ_OS_UNIX

typedef enum PasswdField {
  PASSWD_NAME,
  PASSWD_DIR,
} PasswdField;

/* Copies a field of the password database entry of the effective user. */
static int effective_passwd_field(char* buffer, size_t n, PasswdField field)
{
  const struct passwd* pw;
  int res;

#ifdef HAVE_GETPWUID_R
  struct passwd entry;
  struct passwd* found = NULL;
  long bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
  if (bufsize <= 0)
    bufsize = 16384;

  char* buf = (char*)NQMalloc((size_t)bufsize);
  if (buf == NULL)
    return fail(buffer, n, NQ_ENOMEM);

  /* returns the error number itself; no entry is a success with NULL */
  int ret = getpwuid_r(geteuid(), &entry, buf, (size_t)bufsize, &found);
  pw = found;
  if (ret != 0) {
    NQFree(buf);
    return fail(buffer, n, ret);
  }
#else
  NQSetLastError(0);
  pw = getpwuid(geteuid());
  if (pw == NULL && NQGetLastError() != 0)
    return fail(buffer, n, NQGetLastError());
#endif

  if (pw != NULL)
    res = copy_value(buffer, n, field == PASSWD_NAME ? pw->pw_name : pw->pw_dir);
  else
    res = fail(buffer, n, NQ_ENOENT);

#ifdef HAVE_GETPWUID_R
  NQFree(buf);
#endif
  return res;
}

#endif /* NQ_OS_UNIX */

int NQGetUserName(char* buffer, size_t n)
{
  if (n == 0)
    return -NQ_ENAMETOOLONG;

#if defined(NQ_OS_WINDOWS)
  return token_user_name(buffer, n);

#elif defined(NQ_OS_UNIX) && defined(HAVE_GETLOGIN_R)
  /* returns the error number itself, ERANGE when the name does not fit */
  int ret = getlogin_r(buffer, n);
  if (ret == ERANGE)
    return fail(buffer, n, NQ_ENAMETOOLONG);
  if (ret != 0)
    return fail(buffer, n, ret);
  buffer[n - 1] = '\0';
  return (int)NQStrlen(buffer);

#elif defined(NQ_OS_UNIX)
  NQSetLastError(0);
  const char* name = getlogin();
  if (name == NULL && NQGetLastError() != 0)
    return fail(buffer, n, NQGetLastError());
  return copy_value(buffer, n, name);

#else
  return fail(buffer, n, NQ_ENOTSUPP);
#endif
}

int NQGetEffectiveUserName(char* buffer, size_t n)
{
  if (n == 0)
    return -NQ_ENAMETOOLONG;

#if defined(NQ_OS_WINDOWS)
  return token_user_name(buffer, n);
#elif defined(NQ_OS_UNIX)
  return effective_passwd_field(buffer, n, PASSWD_NAME);
#else
  return fail(buffer, n, NQ_ENOTSUPP);
#endif
}

int NQGetHomeDirectory(char* buffer, size_t n)
{
  if (n == 0)
    return -NQ_ENAMETOOLONG;

#if defined(NQ_OS_WINDOWS)
  WCHAR wbuf[MAX_PATH];
  NQUnicodeInfo info;

  DWORD length = GetEnvironmentVariableW(L"USERPROFILE", wbuf, MAX_PATH);
  if (length == 0)
    return fail(buffer, n, GetLastError() == ERROR_ENVVAR_NOT_FOUND ? NQ_ENOENT : NQGetLastError());
  if (length >= MAX_PATH)
    return fail(buffer, n, NQ_ENAMETOOLONG);

  /* NQPathFrom swaps '\\' for '/', which keeps the UTF-8 length */
  if (!NQCalculateUTF16Info((const uint16_t*)wbuf, (const uint16_t*)wbuf + length, &info))
    return fail(buffer, n, NQ_EINVAL);
  if (info.utf8Size >= n)
    return fail(buffer, n, NQ_ENAMETOOLONG);
  return (int)NQPathFrom(buffer, n, wbuf);

#elif defined(NQ_OS_UNIX)
  const char* home = getenv("HOME");
  if (home != NULL && home[0] != '\0')
    return copy_value(buffer, n, home);
  return effective_passwd_field(buffer, n, PASSWD_DIR);

#else
  return fail(buffer, n, NQ_ENOTSUPP);
#endif
}

int NQIsSuperUser(void)
{
#if defined(NQ_OS_WINDOWS)
  HANDLE token;
  TOKEN_ELEVATION elevation;
  DWORD size = 0;
  int res;

  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    return -NQGetLastError();
  if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size))
    res = elevation.TokenIsElevated != 0;
  else
    res = -NQGetLastError();
  CloseHandle(token);
  return res;

#elif defined(NQ_OS_UNIX)
  return geteuid() == 0;

#else
  return -NQ_ENOTSUPP;
#endif
}

#ifdef NQ_OS_UNIX

/* The error code for a failed getpw*() or getgr*(): those that find no
 * entry leave errno alone, or set it to one of the codes POSIX allows for
 * "not found". */
static int lookup_error(void)
{
  int err = NQGetLastError();
  if (err == 0 || err == ENOENT || err == ESRCH || err == EBADF || err == EPERM)
    return -NQ_ENOENT;
  return -err;
}

#endif /* NQ_OS_UNIX */

int NQGetUserIdByName(const char* name, unsigned* id)
{
#if defined(NQ_OS_UNIX)
  if (name == NULL || *name == '\0')
    return -NQ_ENOENT;
  NQSetLastError(0);
  const struct passwd* pw = getpwnam(name);
  if (pw == NULL)
    return lookup_error();
  *id = (unsigned)pw->pw_uid;
  return 0;
#else
  (void)name;
  (void)id;
  return -NQ_ENOTSUPP;
#endif
}

int NQGetGroupIdByName(const char* name, unsigned* id)
{
#if defined(NQ_OS_UNIX)
  if (name == NULL || *name == '\0')
    return -NQ_ENOENT;
  NQSetLastError(0);
  const struct group* gr = getgrnam(name);
  if (gr == NULL)
    return lookup_error();
  *id = (unsigned)gr->gr_gid;
  return 0;
#else
  (void)name;
  (void)id;
  return -NQ_ENOTSUPP;
#endif
}

int NQGetUserNameById(unsigned id, char* buffer, size_t n)
{
  if (n == 0)
    return -NQ_ENAMETOOLONG;
#if defined(NQ_OS_UNIX)
  NQSetLastError(0);
  const struct passwd* pw = getpwuid((uid_t)id);
  if (pw == NULL)
    return fail(buffer, n, (NQErrorCode)-lookup_error());
  return copy_value(buffer, n, pw->pw_name);
#else
  (void)id;
  return fail(buffer, n, NQ_ENOTSUPP);
#endif
}

int NQGetGroupNameById(unsigned id, char* buffer, size_t n)
{
  if (n == 0)
    return -NQ_ENAMETOOLONG;
#if defined(NQ_OS_UNIX)
  NQSetLastError(0);
  const struct group* gr = getgrgid((gid_t)id);
  if (gr == NULL)
    return fail(buffer, n, (NQErrorCode)-lookup_error());
  return copy_value(buffer, n, gr->gr_name);
#else
  (void)id;
  return fail(buffer, n, NQ_ENOTSUPP);
#endif
}
