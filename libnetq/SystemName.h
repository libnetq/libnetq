/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_SYSTEMNAME_H
#define _LIBNETQ_SYSTEMNAME_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_SYSTEM_NAME_MAX 257

typedef struct NQSystemName {
  char kernelName[NQ_SYSTEM_NAME_MAX];     /* "Linux", "Darwin", "Windows_NT" */
  char nodeName[NQ_SYSTEM_NAME_MAX];       /* the network name of the machine */
  char kernelRelease[NQ_SYSTEM_NAME_MAX];  /* on Windows "MAJOR.MINOR" */
  char kernelVersion[NQ_SYSTEM_NAME_MAX];  /* on Windows the build number */
  char machine[NQ_SYSTEM_NAME_MAX];        /* "x86_64", "aarch64", "arm64" */
  char processor[NQ_SYSTEM_NAME_MAX];      /* empty when unknown */
  char hardwarePlatform[NQ_SYSTEM_NAME_MAX]; /* empty when unknown */
  char operatingSystem[NQ_SYSTEM_NAME_MAX];  /* "GNU/Linux", "Android", "Darwin" */
} NQSystemName;

NQ_EXPORT int NQGetSystemName(NQSystemName* name);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_SYSTEMNAME_H */
