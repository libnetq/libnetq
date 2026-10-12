/*
 * MIT License
 *
 * Copyright (c) 2020-2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_ASSERT_H
#define _LIBNETQ_ASSERT_H

#include <libnetq/Basic.h>
#include <libnetq/Log.h>
#include <libnetq/Abort.h>

#ifdef __cplusplus
extern "C" {
#endif

#if NQ_DEBUG

#define NQ_ASSERT_ENABLED 1
#define NQ_ASSERT_DISABLED 0

#define NQ_ASSERT(condition) do { \
  if (!(condition)) { \
    NQ_ALWAYS_LOGE("Assertion failed: %s", #condition); \
    NQLog_report(NQ_LOG_ERROR, NQ_LOG_TAG, NQ_FILE, NQ_LINE, NQ_PRETTY_FUNCTION); \
    NQAbort(); \
  } \
} while (0)

#define NQ_ASSERT_WITH_MESSAGE(condition, ...) do { \
  if (!(condition)) { \
    NQ_ALWAYS_LOGE("Assertion failed: %s", #condition); \
    NQ_ALWAYS_LOGE(__VA_ARGS__); \
    NQLog_report(NQ_LOG_ERROR, NQ_LOG_TAG, NQ_FILE, NQ_LINE, NQ_PRETTY_FUNCTION); \
    NQAbort(); \
  } \
} while (0)

#define NQ_ASSERT_NOT_REACHED() do { \
  NQ_ALWAYS_LOGE("Should not be reached"); \
  NQLog_report(NQ_LOG_ERROR, NQ_LOG_TAG, NQ_FILE, NQ_LINE, NQ_PRETTY_FUNCTION); \
  NQAbort(); \
} while (0)

#define NQ_ASSERT_UNUSED(variable, condition) NQ_ASSERT(condition)

#define NQ_ALWAYS_ASSERT(condition) NQ_ASSERT(condition)
#define NQ_ALWAYS_ASSERT_WITH_MESSAGE(condition, ...) NQ_ASSERT_WITH_MESSAGE(condition, __VA_ARGS__)
#define NQ_ALWAYS_ASSERT_NOT_REACHED() NQ_ASSERT_NOT_REACHED()

#else

#define NQ_ASSERT_ENABLED 0
#define NQ_ASSERT_DISABLED 1

#define NQ_ASSERT(condition) ((void)0)
#define NQ_ASSERT_WITH_MESSAGE(condition, ...) ((void)0)
#define NQ_ASSERT_NOT_REACHED() ((void)0)
#define NQ_ASSERT_UNUSED(variable, condition) ((void)variable)

#define NQ_ALWAYS_ASSERT(condition) do { \
  if (NQ_UNLIKELY(!(condition))) { \
    NQAbort(); \
  } \
} while (0)
#define NQ_ALWAYS_ASSERT_WITH_MESSAGE(condition, ...) NQ_ALWAYS_ASSERT(condition)
#define NQ_ALWAYS_ASSERT_NOT_REACHED() NQAbort()

#endif /* NQ_DEBUG */

#ifdef __cplusplus
#define NQ_STATIC_ASSERT static_assert
#else
#define _NQ_STATIC_ASSERT(condition, message, prefix, suffix) \
    typedef int prefix ## suffix[(condition) ? 1 : -1] NQ_ALLOW_UNUSED;
#define NQ_STATIC_ASSERT(condition, message) _NQ_STATIC_ASSERT(condition, message, _static_assert_, NQ_COUNTER)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_ASSERT_H */
