/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/Time.h"

#include <libnetq/string/String.h>
#include <libnetq/string/Sprintf.h>
#include <libnetq/ConstExpr.h>
#include <libnetq/Assert.h>

#ifdef NQ_OS_KERNEL
#include <linux/timekeeping.h>
#include <linux/ktime.h>
#endif

#ifdef NQ_OS_WINDOWS
# undef WIN32_LEAN_AND_MEAN
# include <winsock2.h>
# include <windows.h>
#endif

#ifdef NQ_OS_UNIX
# include <sys/time.h>
#endif

#if defined(NQ_OS_ANDROID) && defined(NQ_CPU_32BIT)
# include <time64.h>
#endif

#define WINFTIME_AS_EPOCH_100NS NQ_UINT64_C(116444736000000000)

// UNIX epoch	since January 1, 1970
// NTP epoch since January 1, 1900

#define _TIME_T_MIN (1L << (sizeof(time_t) * NQ_CHAR_BIT - 1))
#define _TIME_T_MAX (~_TIME_T_MIN)

#if !defined(HAVE_GMTIME_R) && (defined(NQ_OS_UNIX) || (defined(_POSIX_THREAD_SAFE_FUNCTIONS) && _POSIX_THREAD_SAFE_FUNCTIONS >= 200112L))
# define HAVE_GMTIME_R 1
#endif

#if !defined(HAVE_GMTIME_S) && defined(NQ_OS_WINDOWS)
# define HAVE_GMTIME_S 1
#endif

static inline NQTimeMs timeValToTimeMs(const NQTimeVal* tv)
{
  return (NQTimeMs)(tv->tv_sec * NQ_MSECS_PER_SEC + tv->tv_usec / NQ_MSECS_PER_SEC);
}

static inline NQTimeMs timeSpecToTimeMs(const NQTimeSpec* ts)
{
  if (ts == NULL)
    return -1; /* infinitely for select or poll */

  return (NQTimeMs)ts->tv_sec * NQ_MSECS_PER_SEC + ts->tv_nsec / NQ_NSECS_PER_MSEC;
}

static inline NQTimeMs winFileTimeToTimeMs(uint32_t highDateTime, uint32_t lowDateTime)
{
  uint64_t t = ((uint64_t)highDateTime << 32) | (uint64_t)lowDateTime;
  return (t - WINFTIME_AS_EPOCH_100NS) / 10000;
}

static inline void winFileTimeToTimeSpec(uint32_t highDateTime, uint32_t lowDateTime, NQTimeSpec* ts)
{
  uint64_t t = ((uint64_t)highDateTime << 32) | (uint64_t)lowDateTime;
  if (t <= WINFTIME_AS_EPOCH_100NS) {
    ts->tv_sec = 0;
    ts->tv_nsec = 0;
  }
  else {
    t -= WINFTIME_AS_EPOCH_100NS;
    ts->tv_sec  = (time_t)(t / 10000000);
    ts->tv_nsec = (long)((t % 10000000) * 100);
  }
}

#ifdef NQ_OS_WINDOWS
# if NQ_CHECK_LEVEL >= 2
static bool WinFileTimeToTimeMsCheck(const FILETIME* ft, NQTimeMs timeMs)
{
  ULARGE_INTEGER dateTime;
  memcpy(&dateTime, ft, sizeof(dateTime));
  return (dateTime.QuadPart - WINFTIME_AS_EPOCH_100NS) / 10000;
}
# else
#  define WinFileTimeToTimeMsCheck(...) true
# endif
#endif

NQTimeMs NQGetTimeMs()
{
#if defined(NQ_OS_KERNEL)
  ktime_t now = ktime_get_real();
  return (NQTimeMs)ktime_to_ms(now);
#elif defined(NQ_OS_WINDOWS)
  FILETIME fileTime;

  GetSystemTimeAsFileTime(&fileTime);
  NQTimeMs timeMs = winFileTimeToTimeMs(fileTime.dwHighDateTime, fileTime.dwLowDateTime);
  NQ_ASSERT(WinFileTimeToTimeMsCheck(&fileTime, timeMs));
  return timeMs;
#else
  struct timeval now;
  gettimeofday(&now, NULL);
  return NQTimeValToTimeMs(&now);
#endif
}

NQTickMs NQGetCPUTickMs()
{
#if defined(NQ_OS_KERNEL)
  ktime_t now = ktime_get();
  return (NQTickMs)ktime_to_ms(now);
#elif defined(NQ_COMPILER_MINGW64)
  return (NQTickMs)GetTickCount();
#elif defined(NQ_OS_WINDOWS)
  return (NQTickMs)GetTickCount64();
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (NQTickMs)ts.tv_sec * 1000 + (NQTickMs)ts.tv_nsec / 1000000;
#endif
}

void NQGetLocaltime(const time_t* t, struct tm* tm)
{
#if defined(HAVE_LOCALTIME_R)
  localtime_r(t, tm);

#elif defined(HAVE_LOCALTIME_S)
# if defined(NQ_OS_WINDOWS)
  localtime_s(tm, t);
# else
  localtime_s(t, tm);
# endif

#elif defined(NQ_OS_KERNEL)
  time64_to_tm(*t, 0, tm);

#elif defined(NQ_OS_WINDOWS)
  FILETIME uTm, lTm;
  SYSTEMTIME pTm;
  int64_t t64;

  if (tm == NULL)
    return;

  t64 = *t;
  t64 = (t64 + 11644473600) * 10000000;
  uTm.dwLowDateTime = (DWORD)(t64 & 0xFFFFFFFF);
  uTm.dwHighDateTime = (DWORD)(t64 >> 32);

  FileTimeToLocalFileTime(&uTm, &lTm);
  FileTimeToSystemTime(&lTm, &pTm);

  tm->tm_year = pTm.wYear - NQ_TM_YEAR_BASE;
  tm->tm_mon = pTm.wMonth - 1;
  tm->tm_wday = pTm.wDayOfWeek;
  tm->tm_mday = pTm.wDay;
  tm->tm_hour = pTm.wHour;
  tm->tm_min = pTm.wMinute;
  tm->tm_sec = pTm.wSecond;

#ifdef HAVE_TM_GMTOFF
  tm->tm_gmtoff = 0; // TODO: FIXME
#endif

#else
# warning NQGetLocaltime() not implemented!
  gmtime_r(t, tm);

#endif
}

time_t nq_timegm(const struct tm* tm)
{
#if defined(NQ_OS_KERNEL)
    return mktime64(tm->tm_year + NQ_TM_YEAR_BASE,
                    tm->tm_mon + 1,
                    tm->tm_mday,
                    tm->tm_hour,
                    tm->tm_min,
                    tm->tm_sec);
#elif defined(NQ_OS_WINDOWS)
  return _mkgmtime((struct tm* const)tm);
#elif defined(NQ_OS_ANDROID) && defined(NQ_CPU_32BIT)
  time64_t result;

  result = timegm64(tm);
  if (result < _TIME_T_MIN || result > _TIME_T_MAX)
    return -1;

  return result;
#else
  return timegm((struct tm*)tm);
#endif
}

#define _MS_PER_SEC    (1000LL)
#define _MS_PER_MIN    (60LL  * _MS_PER_SEC)
#define _MS_PER_HOUR   (60LL  * _MS_PER_MIN)
#define _MS_PER_DAY    (24LL  * _MS_PER_HOUR)
#define _MS_PER_YEAR   (365L  * _MS_PER_DAY)
#define _MS_PER_4YEAR  (1461L * _MS_PER_DAY)

void nq_gmtimems(NQTimeMs time, struct tm* ptm, int* pms)
{
  static const int s_lpdays[] = { -1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
  static const int s_days[] = { -1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364 };
  
  const int* mdays;

  NQTimeMs ctimer = time;
  NQTimeMs tmptimer;
  bool isleapyear = false;
  
  if (pms != NULL)
    *pms = (int)(time % _MS_PER_SEC);

  if (ptm == NULL)
    return;

  tmptimer = (NQTimeMs)(ctimer / _MS_PER_4YEAR);
  ctimer -= ((NQTimeMs)tmptimer * _MS_PER_4YEAR);
  tmptimer = (tmptimer * 4) + 70;

  if (ctimer >= _MS_PER_YEAR) {
    tmptimer++;
    ctimer -= _MS_PER_YEAR;
    if (ctimer >= _MS_PER_YEAR) {
      tmptimer++;
      ctimer -= _MS_PER_YEAR;
      if (ctimer < (_MS_PER_YEAR + _MS_PER_DAY))
        isleapyear = true;
      else {
        tmptimer++;
        ctimer -= (_MS_PER_YEAR + _MS_PER_DAY);
      }
    }
  }

  ptm->tm_year = (int)tmptimer;
  ptm->tm_yday = (int)(ctimer / _MS_PER_DAY);
  ctimer -= (NQTimeMs)(ptm->tm_yday) * _MS_PER_DAY;

  if (isleapyear)
    mdays = s_lpdays;
  else
    mdays = s_days;

  for (tmptimer = 1; mdays[tmptimer] < ptm->tm_yday; tmptimer++)
    /* */;

  ptm->tm_mon = (int)(--tmptimer);
  ptm->tm_mday = (int)(ptm->tm_yday - mdays[tmptimer]);

  ptm->tm_wday = (int)(((time / _MS_PER_DAY) + 4) % 7);
  ptm->tm_hour = (int)(ctimer / _MS_PER_HOUR);
  ctimer -= (NQTimeMs)ptm->tm_hour * _MS_PER_HOUR;
  ptm->tm_min = (int)(ctimer / _MS_PER_MIN);
  ctimer -= (NQTimeMs)ptm->tm_min * _MS_PER_MIN;
  ptm->tm_sec = (int)(ctimer / _MS_PER_SEC);

#ifdef NQ_OS_KERNEL
  /* do nothing */
#else
  ptm->tm_isdst = 0;
#endif
}

void NQGmtime(const time_t* timep, struct tm* result)
{
#if defined(HAVE_GMTIME_R)
  gmtime_r(timep, result);
#elif defined(HAVE_GMTIME_S)
  gmtime_s(result, timep);
#else
  if (timep != NULL)
    nq_gmtimems(((NQTimeMs)*timep) * _MS_PER_SEC, result, NULL);
  else if (result != NULL)
    memset(result, 0, sizeof(*result));
#endif
}

int NQTimeMsFormat(NQTimeMs time, int format, char* buffer, size_t size)
{
  int n, msec;
  struct tm tm;

  switch (format) {
  case NQ_DT_UNIXEPOCH:
    n = snprintf(buffer, size, "%lld", (long long)(time / _MS_PER_SEC));
    break;

  case NQ_DT_UNIXEPOCHMS:
    n = snprintf(buffer, size, "%lld", (long long)(time));
    break;

  case NQ_DT_RFC2445TZ:
    nq_gmtimems(time, &tm, NULL);
    n = snprintf(buffer, size, "%04i%02i%02iT%02i%02i%02iZ",
                 (int)(tm.tm_year + NQ_TM_YEAR_BASE),
                 tm.tm_mon + 1,
                 tm.tm_mday,
                 tm.tm_hour,
                 tm.tm_min,
                 tm.tm_sec);
    break;

  case NQ_DT_RFC3339:
    nq_gmtimems(time, &tm, NULL);
    n = snprintf(buffer, size, "%04i-%02i-%02i %02i:%02i:%02i",
                 (int)(tm.tm_year + NQ_TM_YEAR_BASE),
                 tm.tm_mon + 1,
                 tm.tm_mday,
                 tm.tm_hour,
                 tm.tm_min,
                 tm.tm_sec);
    break;

  case NQ_DT_RFC3339_TIME:
    nq_gmtimems(time, &tm, NULL);
    n = snprintf(buffer, size, "%02i:%02i:%02i",
    tm.tm_hour, tm.tm_min, tm.tm_sec);
    break;

  case NQ_DT_RFC3339_TIMEMS:
    nq_gmtimems(time, &tm, &msec);
    n = snprintf(buffer, size, "%02i:%02i:%02i.%03i",
                 tm.tm_hour,
                 tm.tm_min,
                 tm.tm_sec,
                 msec);
    break;

  default:
    NQ_ASSERT_NOT_REACHED();
    return 0;
  }
  
  return n;
}

void NQDataTime_init(NQDataTime* datatime)
{
  memset(datatime, 0, sizeof(*datatime));
}

void NQDataTime_initLocalTime(NQDataTime* datatime)
{
  NQ_ASSERT_NOT_REACHED();
}

NQTimeVal* NQTimeMsToTimeVal(NQTimeMs time, NQTimeVal* tv)
{
  if (time < 0)
    return NULL;

  if (time == 0) {
    tv->tv_sec = 0;
    tv->tv_usec = 0;
  }
  else {
    tv->tv_sec = time / NQ_MSECS_PER_SEC;
    tv->tv_usec = (time % NQ_MSECS_PER_SEC) * NQ_MSECS_PER_SEC;
  }

  return tv;
}

NQTimeSpec* NQTimeMsToTimeSpec(NQTimeMs time, NQTimeSpec* ts)
{
  if (time < 0)
    return NULL;

  if (time == 0) {
    ts->tv_sec = 0;
    ts->tv_nsec = 0;
  }
  else {
    ts->tv_sec = time / NQ_MSECS_PER_SEC;
    ts->tv_nsec = (time % NQ_MSECS_PER_SEC) * NQ_NSECS_PER_MSEC;
  }

  return ts;
}

NQTimeMs NQTimeValToTimeMs(const NQTimeVal* tv)
{
  if (tv == NULL)
    return -1; /* infinitely for select or poll */

  return timeValToTimeMs(tv);
}

NQTimeMs NQTimeSpecToTimeMs(const NQTimeSpec* ts)
{
  if (ts == NULL)
    return -1; /* infinitely for select or poll */

  return timeSpecToTimeMs(ts);
}

NQTimeMs NQWinFileTimeToTimeMs(uint32_t highDateTime, uint32_t lowDateTime)
{
  return winFileTimeToTimeMs(highDateTime, lowDateTime);
}

void NQWinFileTimeToTimeSpec(uint32_t highDateTime, uint32_t lowDateTime, NQTimeSpec* ts)
{
  winFileTimeToTimeSpec(highDateTime, lowDateTime, ts);
}
