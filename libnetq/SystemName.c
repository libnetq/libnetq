/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/SystemName.h"

#include <libnetq/ErrorCode.h>
#include <libnetq/string/Sprintf.h>
#include <libnetq/string/String.h>

#if defined(NQ_OS_KERNEL)
# include <linux/utsname.h>
#elif defined(NQ_OS_WINDOWS)
# include <windows.h>
# include <libnetq/GetComputerName.h>
#elif defined(NQ_OS_UNIX)
# include <sys/utsname.h>
# if defined(NQ_OS_DARWIN)
#  include <sys/sysctl.h>
# endif
#endif

#define COPY_FIELD(field, text) NQSnprintf(field, sizeof(field), "%s", text)

#if defined(NQ_OS_WINDOWS)

/* Older MinGW headers, such as Qt 5.12's, stop short of ARM64 */
#ifndef PROCESSOR_ARCHITECTURE_ARM64
# define PROCESSOR_ARCHITECTURE_ARM64 12
#endif

typedef LONG(WINAPI* RtlGetVersionFn)(OSVERSIONINFOW*);

/* GetVersionEx answers with the version in the application's manifest;
 * RtlGetVersion with the real one */
static int windows_version(OSVERSIONINFOW* version)
{
  HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
  RtlGetVersionFn get = ntdll ? (RtlGetVersionFn)(void*)GetProcAddress(ntdll, "RtlGetVersion") : NULL;
  if (get == NULL)
    return -NQ_ENOTSUPP;
  NQMemset(version, 0, sizeof(*version));
  version->dwOSVersionInfoSize = sizeof(*version);
  return get(version) == 0 ? 0 : -NQ_ENOTSUPP;
}

/* The names GNU tools use, for what the machine is rather than this process */
static const char* windows_machine(void)
{
  SYSTEM_INFO info;
  GetNativeSystemInfo(&info);
  switch (info.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64: return "x86_64";
    case PROCESSOR_ARCHITECTURE_ARM64: return "aarch64";
    case PROCESSOR_ARCHITECTURE_ARM: return "armv7l";
    case PROCESSOR_ARCHITECTURE_INTEL: return "i686";
    default: return "unknown";
  }
}

#endif

static int get_system_name(NQSystemName* name)
{
#if defined(NQ_OS_KERNEL)
  struct new_utsname* u = init_utsname();
  COPY_FIELD(name->kernelName, u->sysname);
  COPY_FIELD(name->nodeName, u->nodename);
  COPY_FIELD(name->kernelRelease, u->release);
  COPY_FIELD(name->kernelVersion, u->version);
  COPY_FIELD(name->machine, u->machine);
  COPY_FIELD(name->operatingSystem, u->sysname);
  return 0;

#elif defined(NQ_OS_WINDOWS)
  OSVERSIONINFOW version;
  int ret = windows_version(&version);
  if (ret != 0)
    return ret;
  ret = NQGetComputerName(name->nodeName, sizeof(name->nodeName));
  if (ret < 0)
    return ret;
  COPY_FIELD(name->kernelName, "Windows_NT");
  NQSnprintf(name->kernelRelease, sizeof(name->kernelRelease), "%lu.%lu",
             (unsigned long)version.dwMajorVersion, (unsigned long)version.dwMinorVersion);
  NQSnprintf(name->kernelVersion, sizeof(name->kernelVersion), "%lu",
             (unsigned long)version.dwBuildNumber);
  COPY_FIELD(name->machine, windows_machine());
  COPY_FIELD(name->operatingSystem, "Windows");
  return 0;

#elif defined(NQ_OS_UNIX)
  struct utsname u;
  if (uname(&u) < 0)
    return -NQGetLastError();
  COPY_FIELD(name->kernelName, u.sysname);
  COPY_FIELD(name->nodeName, u.nodename);
  COPY_FIELD(name->kernelRelease, u.release);
  COPY_FIELD(name->kernelVersion, u.version);
  COPY_FIELD(name->machine, u.machine);

# if defined(NQ_OS_ANDROID)
  COPY_FIELD(name->operatingSystem, "Android");
# elif defined(NQ_OS_LINUX)
  COPY_FIELD(name->operatingSystem, "GNU/Linux");
# else
  COPY_FIELD(name->operatingSystem, u.sysname);
# endif

# if defined(NQ_OS_LINUX)
  /* the kernel has no separate answer: distributions print the machine */
  COPY_FIELD(name->processor, u.machine);
  COPY_FIELD(name->hardwarePlatform, u.machine);
# elif defined(NQ_OS_DARWIN)
#  if defined(__arm__) || defined(__arm64__)
  COPY_FIELD(name->processor, "arm");
#  elif defined(__i386__) || defined(__x86_64__)
  COPY_FIELD(name->processor, "i386");
#  elif defined(__ppc__) || defined(__ppc64__)
  COPY_FIELD(name->processor, "powerpc");
#  endif
  size_t size = sizeof(name->hardwarePlatform);
  if (sysctlbyname("hw.model", name->hardwarePlatform, &size, NULL, 0) != 0)
    name->hardwarePlatform[0] = '\0';
# endif
  return 0;

#else
  return -NQ_ENOTSUPP;

#endif
}

int NQGetSystemName(NQSystemName* name)
{
  NQMemset(name, 0, sizeof(*name));
  int ret = get_system_name(name);
  if (ret != 0)
    NQMemset(name, 0, sizeof(*name));
  return ret;
}
