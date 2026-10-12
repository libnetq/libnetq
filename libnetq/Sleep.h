/*
 * MIT License
 *
 * Copyright (c) 2020-2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_SLEEP_H
#define _LIBNETQ_SLEEP_H

#include <libnetq/Basic.h>
#include <libnetq/Time.h>

#ifdef __cplusplus
extern "C" {
#endif

NQ_EXPORT void NQSleep(int64_t ms);
NQ_EXPORT void NQSleepTimeSpec(const NQTimeSpec* duration);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_SLEEP_H */
