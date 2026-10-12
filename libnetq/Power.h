/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_POWER_H
#define _LIBNETQ_POWER_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum NQPowerAction {
  NQ_POWER_OFF = 0, /* shut the system down and switch the machine off */
  NQ_POWER_REBOOT,  /* shut the system down and restart it */
  NQ_POWER_HALT,    /* shut the system down, leaving the machine on */
} NQPowerAction;

/* Skip the orderly shutdown: tell the kernel directly, at once, without
 * asking the session manager (systemd-logind, loginwindow) to stop the
 * services and close the sessions first; on Windows applications are
 * closed without asking them. Usually needs root, except on Windows. */
#define NQ_POWER_FORCE   0x1
/* With NQ_POWER_FORCE: do not flush the file systems first */
#define NQ_POWER_NO_SYNC 0x2

NQ_EXPORT int NQPowerControl(NQPowerAction action, unsigned flags);
NQ_EXPORT int NQSetRebootParameter(const char* parameter);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_POWER_H */
