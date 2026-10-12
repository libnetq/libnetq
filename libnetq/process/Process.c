/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/process/Process.h"

#include <libnetq/ErrorCode.h>

#if defined(NQ_OS_WINDOWS)
# include <windows.h>
#elif defined(NQ_OS_UNIX)
# include <signal.h>
# include <sys/types.h>
#endif

int NQIsProcessRunning(int64_t pid)
{
#if defined(NQ_OS_WINDOWS)
  if (pid < 0 || pid > (int64_t)0xffffffffu)
    return 0;
  HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, (DWORD)pid);
  if (!h) {
    DWORD ec = GetLastError();
    if (ec == ERROR_ACCESS_DENIED)
      return 1; /* there, but not ours to open */
    return ec == ERROR_INVALID_PARAMETER ? 0 : -(int)ec;
  }
  /* an exited process lives on while handles to it are open */
  DWORD wait = WaitForSingleObject(h, 0);
  CloseHandle(h);
  return wait == WAIT_TIMEOUT;

#elif defined(NQ_OS_UNIX)
  if (pid != (int64_t)(pid_t)pid)
    return 0;
  if (kill((pid_t)pid, 0) == 0)
    return 1;
  if (errno == EPERM)
    return 1; /* there, but not ours to signal */
  return errno == ESRCH ? 0 : -errno;

#else
  NQ_UNUSED_PARAM(pid);
  return -NQ_ENOTSUPP;

#endif
}

#if defined(NQ_OS_WINDOWS)

/* The Linux numbers of the signals whose default action is not to
 * terminate */
# define WIN_SIGCHLD  17
# define WIN_SIGCONT  18
# define WIN_SIGSTOP  19
# define WIN_SIGTSTP  20
# define WIN_SIGTTIN  21
# define WIN_SIGTTOU  22
# define WIN_SIGURG   23
# define WIN_SIGWINCH 28
# define WIN_SIGMAX   31

typedef LONG (NTAPI *NtProcessFunc)(HANDLE);

/* ntdll's NtSuspendProcess or NtResumeProcess on `h` */
static int suspend_resume(HANDLE h, const char* name)
{
  HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
  NtProcessFunc func = ntdll ? (NtProcessFunc)(void (*)(void))GetProcAddress(ntdll, name) : NULL;
  if (!func)
    return -NQ_ENOTSUPP;
  return func(h) < 0 ? -NQ_EPERM : 0;
}

#endif

int NQSendSignal(int64_t pid, int signal)
{
#if defined(NQ_OS_WINDOWS)
  if (signal < 0 || signal > WIN_SIGMAX)
    return -NQ_EINVAL;
  if (pid <= 0)
    return -NQ_ENOTSUPP; /* no process groups */
  if (pid > (int64_t)0xffffffffu)
    return -NQ_ESRCH;

  DWORD access = SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION;
  switch (signal) {
    case 0:
    case WIN_SIGCHLD:
    case WIN_SIGURG:
    case WIN_SIGWINCH:
      break;
    case WIN_SIGCONT:
    case WIN_SIGSTOP:
    case WIN_SIGTSTP:
    case WIN_SIGTTIN:
    case WIN_SIGTTOU:
      access |= PROCESS_SUSPEND_RESUME;
      break;
    default:
      access |= PROCESS_TERMINATE;
      break;
  }

  HANDLE h = OpenProcess(access, FALSE, (DWORD)pid);
  if (!h) {
    DWORD ec = GetLastError();
    if (ec == ERROR_ACCESS_DENIED)
      return -NQ_EPERM;
    return ec == ERROR_INVALID_PARAMETER ? -NQ_ESRCH : -(int)ec;
  }
  int ret = 0;
  /* an exited process lives on while handles to it are open */
  if (WaitForSingleObject(h, 0) != WAIT_TIMEOUT) {
    ret = -NQ_ESRCH;
  } else {
    switch (signal) {
      case WIN_SIGCONT:
        ret = suspend_resume(h, "NtResumeProcess");
        break;
      case WIN_SIGSTOP:
      case WIN_SIGTSTP:
      case WIN_SIGTTIN:
      case WIN_SIGTTOU:
        ret = suspend_resume(h, "NtSuspendProcess");
        break;
      default:
        if (!(access & PROCESS_TERMINATE))
          break; /* 0, or a signal ignored by default */
        if (!TerminateProcess(h, 128 + (UINT)signal)) {
          DWORD ec = GetLastError();
          ret = ec == ERROR_ACCESS_DENIED ? -NQ_EPERM : -(int)ec;
        }
        break;
    }
  }
  CloseHandle(h);
  return ret;

#elif defined(NQ_OS_UNIX)
  if (pid != (int64_t)(pid_t)pid)
    return -NQ_ESRCH;
  return kill((pid_t)pid, signal) == 0 ? 0 : -errno;

#else
  NQ_UNUSED_PARAM(pid);
  NQ_UNUSED_PARAM(signal);
  return -NQ_ENOTSUPP;

#endif
}

int NQQueueSignal(int64_t pid, int signal, int value)
{
#if defined(NQ_OS_LINUX)
  if (pid != (int64_t)(pid_t)pid)
    return -NQ_ESRCH;
  union sigval sv;
  sv.sival_int = value;
  return sigqueue((pid_t)pid, signal, sv) == 0 ? 0 : -errno;

#else
  NQ_UNUSED_PARAM(pid);
  NQ_UNUSED_PARAM(signal);
  NQ_UNUSED_PARAM(value);
  return -NQ_ENOTSUPP;

#endif
}
