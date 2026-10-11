/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_TIME_H
#define _LIBNETQ_TIME_H

#include <libnetq/Basic.h>
#include <libnetq/Limits.h>

#ifdef NQ_OS_KERNEL
#include <linux/time.h>
typedef time64_t time_t;
typedef struct tm NQDataTime;
typedef struct timespec64 NQTimeSpec;
typedef struct NQTimeVal NQTimeVal;
struct NQTimeVal {
  time64_t tv_sec;
  long tv_usec;
};
#elif defined(NQ_OS_WINDOWS) || defined(NQ_OS_UNIX) || defined(NQCONFIG_USE_TIME_H)
#include <time.h>
typedef struct tm NQDataTime;
typedef struct timespec NQTimeSpec;
typedef struct timeval NQTimeVal;
#else
typedef int64_t time_t;
typedef struct NQTimeSpec NQTimeSpec;
struct NQTimeSpec {
  time_t tv_sec;
  long tv_nsec;
};
typedef struct NQTimeVal NQTimeVal;
struct NQTimeVal {
  time_t tv_sec;
  long tv_usec;
};
typedef struct NQDataTime NQDataTime;
struct NQDataTime {
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  long tm_year;
  int tm_wday;
  int tm_yday;
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_HOURS_PER_DAY  24
#define NQ_SECS_PER_MIN   60
#define NQ_MINS_PER_HOUR  60
#define NQ_MSECS_PER_SEC  1000
#define NQ_NSECS_PER_MSEC 1000000
#define NQ_DAYS_PER_WEEK  7
#define NQ_DAYS_PER_NYEAR 365
#define NQ_DAYS_PER_LYEAR 366
#define NQ_SECS_PER_HOUR (NQ_SECS_PER_MIN * NQ_MINS_PER_HOUR)
#define NQ_MONS_PER_YEAR  12
#define NQ_TM_YEAR_BASE   1900
#define NQ_EPOCH_YEAR     1970

#define NQIsLeapYear(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))

#define NQ_TIME_MAX NQ_INT64_MAX

typedef int64_t NQTimeMs;
typedef int64_t NQMonoTime;
typedef uint64_t NQTickMs;
typedef double NQSeconds;
// NQMonoTime

NQ_EXPORT NQTimeMs NQGetTimeMs(void);

static inline int64_t NQGetTimeSec(void)
{
  return NQGetTimeMs() / NQ_MSECS_PER_SEC;
}

static inline int64_t NQGetUnixTime(void)
{
  return NQGetTimeSec();
}

NQ_EXPORT NQTickMs NQGetCPUTickMs(void);

NQ_EXPORT void NQGetLocaltime(const time_t* t, NQDataTime* tm);
NQ_EXPORT void nq_gmtimems(NQTimeMs time, NQDataTime* ptm, int* pms);
NQ_EXPORT void NQGmtime(const time_t* t, NQDataTime* tm);
NQ_EXPORT time_t nq_timegm(const NQDataTime* tm);

enum NQTimeFormat {
  NQ_DT_UNIXEPOCH,           // 1539993600
  NQ_DT_UNIXEPOCHMS,         // 1539993600020
  NQ_DT_RFC2445,             // 19980119 070000
  NQ_DT_RFC2445TZ,           // 19980119T070000Z
  NQ_DT_RFC3339,             // 2019-10-12 07:20:50
  NQ_DT_RFC3339TZ,           // 2019-10-12T07:20:50.529Z
  NQ_DT_RFC3339_TIME,        // 07:20:50
  NQ_DT_RFC3339_TIMEMS,      // 07:20:50.433
                              // 2019-10-12T07:20:50Z (ISO8601)
};

/*
  NQ_DT_RFC2445 | NQ_DT_DATE | NQ_DT_TIME | NQ_DT_MS | NQ_DT_TZ
  NQ_DT_RFC3339 | NQ_DT_DATE | NQ_DT_TIME | NQ_DT_MS | NQ_DT_TZ
*/

NQ_EXPORT int NQTimeMsFormat(NQTimeMs time, int format, char* buffer, size_t size);
NQ_EXPORT NQTimeVal* NQTimeMsToTimeVal(NQTimeMs time, NQTimeVal* tv);
NQ_EXPORT NQTimeSpec* NQTimeMsToTimeSpec(NQTimeMs time, NQTimeSpec* ts);
NQ_EXPORT NQTimeMs NQTimeValToTimeMs(const NQTimeVal* tv);
NQ_EXPORT NQTimeMs NQTimeSpecToTimeMs(const NQTimeSpec* ts);
NQ_EXPORT NQTimeMs NQWinFileTimeToTimeMs(uint32_t highDateTime, uint32_t lowDateTime);
NQ_EXPORT void NQWinFileTimeToTimeSpec(uint32_t highDateTime, uint32_t lowDateTime, NQTimeSpec* ts);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_TIME_H */
