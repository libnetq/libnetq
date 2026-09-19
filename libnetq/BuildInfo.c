/*
 * MIT License
 *
 * Copyright (c) 2020-2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/BuildInfo.h"

#include <libnetq/CPU.h>
#include <libnetq/Compiler.h>
#include <libnetq/string/Sprintf.h>

const char* NQGetBuildCPU(void)
{
  return NQ_CPU_NAME;
}

const char* NQGetBuildABI(void)
{
  return NQ_CPU_ABI;
}

#if defined(NQ_OS_WINDOWS)
#define USERAGENT_PLATFORM "Windows"
#elif defined(NQ_OS_ANDROID)
#define USERAGENT_PLATFORM "Linux Android"
#elif defined(NQ_OS_LINUX)
#define USERAGENT_PLATFORM "Linux"
#elif defined(NQ_OS_DARWIN)
#define USERAGENT_PLATFORM "Darwin"
#elif defined(NQ_OS_UNIX)
#define USERAGENT_PLATFORM "Unix"
#else
#define USERAGENT_PLATFORM "Unknown"
#endif

int NQPutBuildUserAgent(const char* product, const char* version, char* buf, size_t len)
{
  if (version && *version != '\0')
    return snprintf(buf, len, "%s/%s (%s) %s", product, version, USERAGENT_PLATFORM, NQ_CPU_NAME);
  return snprintf(buf, len, "%s (%s) %s", product, USERAGENT_PLATFORM, NQ_CPU_NAME);
}
