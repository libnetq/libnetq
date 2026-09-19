/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/Log.h"

#include <libnetq/string/String.h>
#include <libnetq/string/Sprintf.h>
#include <libnetq/MinMax.h>
#include <libnetq/Time.h>
#include <libnetq/Abort.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/Assert.h>
#include <libnetq/fs/FileHandle.h>
#include <libnetq/ErrorCode.h>

#ifdef NQ_OS_ANDROID
#include <android/log.h>
#endif

#ifdef NQ_OS_WINDOWS
#include <windows.h>
#endif

#ifdef NQ_COMPILER_MSVC
#include <crtdbg.h>
#endif

#ifdef NQ_ENV_LOGFD
#include <libnetq/Env.h>
#include <libnetq/Strtox.h>
#include <libnetq/Limits.h>
#endif

#define BUFFER_SIZE 1024
#define DEFAULT_FILEHANDLE NQGetStdHandle(NQ_STDERR_FILENO)

#ifdef NQ_USE_LOGHANDLER
static int logNoop(void* userdata, NQLogLevel level, const char* tag, const char* format, va_list args)
{
  return -NQ_ENOENT;
}

static NQLogHandler s_handler = logNoop;
static void* s_userdata = NULL;

#endif

char NQLogLevelToChar(NQLogLevel level)
{
  switch (level) {
  case NQ_LOG_INFO:
    return 'I';

  case NQ_LOG_WARNING:
    return 'W';

  case NQ_LOG_ERROR:
    return 'E';

  case NQ_LOG_DEBUG:
    return 'D';

  default:
    NQ_ASSERT_NOT_REACHED();
    return '?';

  };
}

static int printLogPrefix(char* buffer, size_t size, NQLogLevel level, const char* tag)
{
  int n;
  char* ptr;
  char* end;
  size_t len;

  ptr = buffer;
  end = ptr + size;

  n = NQTimeMsFormat(NQGetTimeMs(), NQ_DT_RFC3339, buffer, size);
  ptr += n;

  if (ptr < end)
    *ptr = ' ';
  ptr++;

  if (ptr < end)
    *ptr = NQLogLevelToChar(level);
  ptr++;

  if (ptr < end)
    *ptr = ' ';
  ptr++;

  len = strlen(tag);
  if (ptr < end)
    memcpy(ptr, tag, NQGetMin((size_t)(end - ptr), len));
  ptr += len;

  if (ptr < end)
    *ptr = ' ';
  ptr++;

  return (int)(ptr - buffer);
}

int NQLog_snprint(char* buffer, size_t size, NQLogLevel level, const char* tag, const char* format, ...)
{
  int result;
  va_list args;
  va_start(args, format);
  result = NQLog_vsnprint(buffer, size, level, tag, format, args);
  va_end(args);
  return result;
}

int NQLog_vsnprint(char* buffer, size_t size, NQLogLevel level, const char* tag, const char* format, va_list args)
{
  int n;
  char* ptr;
  char* end;
  size_t len;

  ptr = buffer;
  end = ptr + size;

  n = printLogPrefix(buffer, size, level, tag);
  if (n < 0)
    return n;
  ptr += n;

  len = ptr < end ? end - ptr : 0;
  n = vsnprintf(ptr, len, format, args);
  if (n < 0)
    return n;
  ptr += n;

  return (int)(ptr - buffer);
}

int NQLog_print(NQLogLevel level, const char* tag, const char* format, ...)
{
  int result;
  va_list args;
  va_start(args, format);
  result = NQLog_vprint(level, tag, format, args);
  va_end(args);
  return result;
}

int NQLog_vprint(NQLogLevel level, const char* tag, const char* format, va_list args)
{
#if defined(NQ_USE_LOGHANDLER)
  va_list vargs;
  va_copy(vargs, args);
  int ret = s_handler(s_userdata, level, tag, format, vargs);
  va_end(vargs);
  return ret;

#elif defined(NQ_OS_KERNEL)
  int ret;
  struct va_format vaf;
  va_list vargs;

  va_copy(vargs, args);

  vaf.fmt = format;
  vaf.va = &vargs;

  switch (level) {
  case NQ_LOG_INFO:
    ret = printk(KERN_INFO "%s: %pV\n", tag, &vaf);
    break;
  case NQ_LOG_WARNING:
    ret = printk(KERN_WARNING "%s: %pV\n", tag, &vaf);
    break;
  case NQ_LOG_ERROR:
    ret = printk(KERN_ERR "%s: %pV\n", tag, &vaf);
    break;
  case NQ_LOG_DEBUG:
    ret = printk(KERN_DEBUG "%s: %pV\n", tag, &vaf);
    break;
  default:
    ret = printk(KERN_DEFAULT "%s: %pV\n", tag, &vaf);
    break;
  }

  va_end(vargs);
  return ret;

#elif defined(NQ_OS_ANDROID) && !defined(NQ_ENV_LOGFD)
  // adb logcat -s "NQ"
  switch (level) {
  case NQ_LOG_INFO:
    return __android_log_vprint(ANDROID_LOG_INFO, tag, format, args);
  case NQ_LOG_WARNING:
    return __android_log_vprint(ANDROID_LOG_WARN, tag, format, args);
  case NQ_LOG_ERROR:
    return __android_log_vprint(ANDROID_LOG_ERROR, tag, format, args);
  case NQ_LOG_DEBUG:
    return __android_log_vprint(ANDROID_LOG_DEBUG, tag, format, args);
  default:
    return __android_log_vprint(ANDROID_LOG_DEFAULT, tag, format, args);
  }

#else
  char buffer[BUFFER_SIZE];
  int len = NQLog_vsnprint(buffer, sizeof(buffer), level, tag, format, args);

  char* ptr;
  if (len + 2 > sizeof(buffer))
    ptr = buffer + sizeof(buffer) - 2;
  else {
    ptr = buffer + len;
    while (true) {
      char ch = ptr[-1];
      if (ch != '\n' && ch != '\r')
        break;
      ptr--;
    }
  }

  *ptr++ = '\n';
  *ptr = '\0';

  len = (int)(ptr - buffer);

#if defined(NQ_COMPILER_MSVC) && !defined(NQ_ENV_LOGFD)
  OutputDebugStringA(buffer);
  return len;
#else

#if defined(NQ_ENV_LOGFD)
  static bool s_init = false;
  static NQFileHandle kLogFileHandle = NQ_INVALID_FILE;
  if (NQ_UNLIKELY(!s_init)) {
    kLogFileHandle = DEFAULT_FILEHANDLE;
    char buf[32];
    int rc = NQEnvGet(NQ_ENV_LOGFD, buf, sizeof(buf));
    if (rc > 0 && rc < sizeof(buf)) {
      char* end;
      long num = NQSimpleStrtol(buf, &end, 10);
      if (*end == '\0' && NQ_INT32_MIN <= num && num <= NQ_INT32_MAX)
        kLogFileHandle = NQGetStdHandle((int)num);
    }
    s_init = true;
  }
#else
# define kLogFileHandle DEFAULT_FILEHANDLE
#endif

  return NQFileWrite(kLogFileHandle, buffer, len);
#endif

#endif
}

void NQLog_report(NQLogLevel level, const char* tag, const char* file, int line, const char* function)
{
#if defined(_WIN32) && defined(_DEBUG)
  switch (level) {
  case NQ_LOG_ERROR:
    _CrtDbgReport(_CRT_ERROR, file, line, NULL, "%s\n", function);
    break;
  case NQ_LOG_DEBUG:
  case NQ_LOG_INFO:
  case NQ_LOG_WARNING:
  default:
    _CrtDbgReport(_CRT_WARN, file, line, NULL, "%s\n", function);
    break;
  };
#else
  NQLog_print(level, tag, "%s(%d) : %s", file, line, function);
#endif
}

void NQLog_assert(const char* condition, const char* tag, const char* format, ...)
{
  if (condition)
    NQLog_print(NQ_LOG_ERROR, tag, "Assertion failed: %s", condition);
  else
    NQLog_print(NQ_LOG_ERROR, tag, "Should not be reached");

  if (format) {
    va_list args;
    va_start(args, format);
    NQLog_vprint(NQ_LOG_ERROR, tag, format, args);
    va_end(args);
  }

  NQAbort();
}

void NQLogSetHandler(NQLogHandler handler, void* userdata)
{
#ifdef NQ_USE_LOGHANDLER
  s_handler = handler;
  s_userdata = userdata;
#endif
}
