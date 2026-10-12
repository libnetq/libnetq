/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/Sleep.h"

#include <libnetq/Assert.h>

#if defined(NQ_OS_KERNEL)
# include <linux/delay.h>
#elif defined(NQ_OS_WINDOWS)
# include <windows.h>
# ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#  define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
# endif
#elif defined(NQ_OS_POSIX)
# include <libnetq/Time.h>
# include <libnetq/ErrorCode.h>
# if _POSIX_C_SOURCE >= 199309L
#  include <time.h>
#  define HAVE_NANOSLEEP 1
# else
#  include <unistd.h>
#  define HAVE_USLEEP 1
# endif
#endif

#define NQ_NSECS_PER_SEC ((int64_t)NQ_MSECS_PER_SEC * NQ_NSECS_PER_MSEC)

/* Longest part of a duration slept with one call where the system call
 * has a limit: a day */
#define NQ_SLEEP_PART_SECS ((int64_t)NQ_HOURS_PER_DAY * NQ_SECS_PER_HOUR)

void NQSleep(int64_t ms)
{
  NQ_ASSERT(ms >= 0);
#if defined(NQ_OS_KERNEL)
  msleep_interruptible(ms);
#elif defined(NQ_OS_WINDOWS)
  NQ_ASSERT(ms <= MAXDWORD);
  Sleep((DWORD)ms);
#elif defined(HAVE_NANOSLEEP)
  struct timespec ts;
  ts.tv_sec = ms / NQ_MSECS_PER_SEC;
  ts.tv_nsec = (ms % NQ_MSECS_PER_SEC) * 1000000;
  nanosleep(&ts, NULL);
#elif defined(HAVE_USLEEP)
  usleep(ms * NQ_MSECS_PER_SEC);
#elif defined(NQ_OS_POSIX)
  NQTimeVal tv;
  int rc = select(0, NULL, NULL, NULL, NQTimeMsToTimeVal(ms, &tv));
  NQ_ASSERT_UNUSED(rc, rc == 0);
#else
  // TODO
#endif
}

#if defined(NQ_OS_KERNEL) || defined(NQ_OS_WINDOWS)
/* Sleeps `sec` seconds and `nsec` nanoseconds in milliseconds, rounded up
 * so that the sleep is never shorter than asked */
static void SleepMsParts(int64_t sec, long nsec)
{
  int64_t ms = (nsec + NQ_NSECS_PER_MSEC - 1) / NQ_NSECS_PER_MSEC;
  for (;;) {
    int64_t part = sec < NQ_SLEEP_PART_SECS ? sec : NQ_SLEEP_PART_SECS;
    sec -= part;
    ms += part * NQ_MSECS_PER_SEC;
    if (ms == 0 && sec == 0)
      break;
# if defined(NQ_OS_KERNEL)
    msleep_interruptible((unsigned int)ms);
# else
    Sleep((DWORD)ms);
# endif
    ms = 0;
  }
}
#endif

#if defined(NQ_OS_WINDOWS)
/* Sleeps on a waitable timer, in units of 100 ns; a high-resolution one,
 * where Windows has them (10 1803 and later), is not tied to the 15.6 ms
 * timer tick. Returns 0, or -1 when no timer could be made. */
static int SleepWaitableTimer(int64_t sec, long nsec)
{
  HANDLE timer = CreateWaitableTimerExW(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                        TIMER_ALL_ACCESS);
  if (!timer)
    timer = CreateWaitableTimerW(NULL, TRUE, NULL);
  if (!timer)
    return -1;

  int64_t ticks = (nsec + 99) / 100;
  for (;;) {
    int64_t part = sec < NQ_SLEEP_PART_SECS ? sec : NQ_SLEEP_PART_SECS;
    sec -= part;
    ticks += part * 10000000;
    if (ticks == 0 && sec == 0)
      break;
    LARGE_INTEGER due;
    due.QuadPart = -ticks; /* negative: relative to now */
    if (!SetWaitableTimer(timer, &due, 0, NULL, NULL, FALSE)) {
      CloseHandle(timer);
      SleepMsParts(sec + ticks / 10000000, (long)(ticks % 10000000) * 100);
      return 0;
    }
    WaitForSingleObject(timer, INFINITE);
    ticks = 0;
  }
  CloseHandle(timer);
  return 0;
}
#endif

void NQSleepTimeSpec(const NQTimeSpec* duration)
{
  NQ_ASSERT(duration->tv_nsec >= 0 && duration->tv_nsec < NQ_NSECS_PER_SEC);
  if (duration->tv_sec < 0)
    return;
#if defined(NQ_OS_KERNEL)
  SleepMsParts(duration->tv_sec, duration->tv_nsec);
#elif defined(NQ_OS_WINDOWS)
  if (SleepWaitableTimer(duration->tv_sec, duration->tv_nsec) != 0)
    SleepMsParts(duration->tv_sec, duration->tv_nsec);
#elif defined(HAVE_NANOSLEEP)
  struct timespec ts = *duration;
  while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {
  }
#elif defined(HAVE_USLEEP)
  int64_t sec = duration->tv_sec;
  int64_t usec = (duration->tv_nsec + 999) / 1000;
  /* usleep may refuse a second or more; a part cut short by a signal is
   * slept again */
  while (sec > 0 || usec > 0) {
    if (usec == 0) {
      sec--;
      usec = 1000000;
    }
    useconds_t part = usec < 1000000 ? (useconds_t)usec : 999999;
    if (usleep(part) == 0)
      usec -= part;
  }
#else
  // TODO
#endif
}
