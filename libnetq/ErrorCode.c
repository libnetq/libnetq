/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/ErrorCode.h"

#include <libnetq/string/Sprintf.h>
#include <libnetq/string/StringUtil.h>
#include <libnetq/Limits.h>
#include <libnetq/MinMax.h>
#include <libnetq/Assert.h>

#ifdef NQ_OS_WINDOWS

#include <libnetq/UTF.h>
#include <windows.h>

NQErrorCode NQGetLastError(void)
{
  return (NQErrorCode)GetLastError();
}

void NQSetLastError(NQErrorCode ec)
{
  SetLastError((DWORD)ec);
}

#endif

int NQGetErrorMessage(NQErrorCode code, char* buf, size_t len)
{
  int ret;

  if (code < 0)
    code = -code;

  if (buf == NULL) {
    NQ_ASSERT(len == 0);
    buf = (char*)&len;
    len = 0;
  }

#if defined(NQ_OS_UNIX)

# ifdef HAVE_STRERROR_R_STR
  const char* message = strerror_r(code, buf, len);
  ret = (int)NQStrlen(message);
  if (message != buf)
    NQMemcpy(buf, message, NQGetMin(ret, len));
# else
  ret = strerror_r(code, buf, len);
  if (ret == NQ_ERANGE) {
    char local[256];
    ret = strerror_r(code, local, sizeof(local));
  }
  ret = ret ? -ret : (int)NQStrlen(buf);
# endif

#elif defined(NQ_OS_WINDOWS)
  NQWChar wide[256];
  DWORD n = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, (DWORD)code,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), wide, NQ_ARRAY_LENGTH(wide), NULL);
  if (n == 0)
    return -NQGetLastError();

  while (n > 0 && (wide[n - 1] == L'\r' || wide[n - 1] == L'\n' || wide[n - 1] == L'.' || wide[n - 1] == L' '))
    n--;

  NQUnicodeInfo info;
  if (!NQCalculateUTF16Info((const uint16_t*)wide, (const uint16_t*)wide + n, &info))
    ret = 0;
  else {
    NQConvertUTF16ToUTF8((const uint16_t*)wide, (const uint16_t*)wide + n, (uint8_t*)buf, (uint8_t*)buf + len, &info);
    ret = (int)info.utf8Size;
  }
#endif

  if (ret == 0)
    return NQSnprintf(buf, len, "error %i", code);

  if (ret < NQGetMin(len, NQ_INT32_MAX)) {
    NQ_ASSERT(buf[ret] == '\0');
    buf[ret] = '\0';
  }

  return ret;
}
