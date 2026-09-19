/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_LOG_H
#define _LIBNETQ_LOG_H

#include <libnetq/VA.h>
#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NQ_LOG_TAG
#define NQ_LOG_TAG "NQ"
#endif

typedef enum NQLogLevel {
  NQ_LOG_INFO,
  NQ_LOG_WARNING,
  NQ_LOG_ERROR,
  NQ_LOG_DEBUG,
} NQLogLevel;

NQ_EXPORT char NQLogLevelToChar(NQLogLevel level);

typedef int (*NQLogHandler) (void* userdata, NQLogLevel level, const char* tag, const char* format, va_list args);
NQ_EXPORT void NQLogSetHandler(NQLogHandler handler, void* userdata);

NQ_EXPORT int NQLog_snprint(char* buffer, size_t size, NQLogLevel level, const char* tag, const char* format, ...) NQ_ATTRIBUTE_PRINTF(5, 6);
NQ_EXPORT int NQLog_vsnprint(char* buffer, size_t size, NQLogLevel level, const char* tag, const char* format, va_list args) NQ_ATTRIBUTE_PRINTF(5, 0);

NQ_EXPORT int NQLog_print(NQLogLevel level, const char* tag, const char* format, ...) NQ_ATTRIBUTE_PRINTF(3, 4);
NQ_EXPORT int NQLog_vprint(NQLogLevel level, const char* tag, const char* format, va_list args) NQ_ATTRIBUTE_PRINTF(3, 0);

NQ_EXPORT void NQLog_report(NQLogLevel level, const char* tag, const char* file, int line, const char* function);
NQ_EXPORT NQ_NORETURN void NQLog_assert(const char* condition, const char* tag, const char* format, ...) NQ_ATTRIBUTE_PRINTF(3, 4);

#define NQ_ALWAYS_LOGI(...) NQLog_print(NQ_LOG_INFO,    NQ_LOG_TAG, __VA_ARGS__)
#define NQ_ALWAYS_LOGW(...) NQLog_print(NQ_LOG_WARNING, NQ_LOG_TAG, __VA_ARGS__)
#define NQ_ALWAYS_LOGE(...) NQLog_print(NQ_LOG_ERROR,   NQ_LOG_TAG, __VA_ARGS__)

#if defined(NDEBUG) || !defined(NQ_LOG_TAG)
#define NQ_LOGI(...) ((void)0)
#define NQ_LOGW(...) ((void)0)
#define NQ_LOGE(...) ((void)0)
#else
#define NQ_LOGI(...) NQ_ALWAYS_LOGI(__VA_ARGS__)
#define NQ_LOGW(...) NQ_ALWAYS_LOGW(__VA_ARGS__)
#define NQ_LOGE(...) NQ_ALWAYS_LOGE(__VA_ARGS__)
#endif

// NQQuietLog_print only text print

#ifdef __cplusplus
}
#endif

#endif /* !_NQ_LOG_H_ */
