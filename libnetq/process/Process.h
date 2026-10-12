/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_PROCESS_PROCESS_H
#define _LIBNETQ_PROCESS_PROCESS_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

NQ_EXPORT int NQIsProcessRunning(int64_t pid);
NQ_EXPORT int NQSendSignal(int64_t pid, int signal);
NQ_EXPORT int NQQueueSignal(int64_t pid, int signal, int value);

typedef struct NQProcessInfo {
  int64_t pid;
  int64_t ppid;
  int64_t pgid;
  int64_t sid;
  int64_t tpgid;      /* foreground process group of its terminal, -1 none */
  int64_t ruid;       /* -1 where there are no numeric IDs (Windows) */
  int64_t euid;
  int64_t rgid;
  int64_t egid;
  char user[64];      /* effective user name */
  char tty[32];       /* controlling terminal without "/dev/", "" none */
  char name[64];      /* command name */
  char wchan[64];     /* kernel function it sleeps in, "" none */
  char* args;         /* arguments joined with spaces, NULL if unknown */
  char state;         /* R S D I T t Z X, as Linux shows them */
  int nice;
  int priority;       /* Linux's: 20 + nice for normal processes */
  int rtprio;
  int policy;
  int processor;
  int threads;
  unsigned flags;     /* Linux PF_* flags, 0 elsewhere */
  uint64_t vsize;     /* bytes */
  uint64_t rss;       /* bytes */
  uint64_t locked;    /* bytes */
  uint64_t text;      /* bytes */
  uint64_t minflt;
  uint64_t majflt;
  int64_t startTime;  /* ms since the epoch, 0 unknown */
  int64_t cpuTime;    /* user + system, ms */
} NQProcessInfo;

NQ_EXPORT int64_t NQGetProcessId(void);
NQ_EXPORT int NQGetProcessList(NQProcessInfo** list, size_t* count);
NQ_EXPORT void NQFreeProcessList(NQProcessInfo* list, size_t count);
NQ_EXPORT uint64_t NQGetPhysicalMemorySize(void);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_PROCESS_PROCESS_H */
