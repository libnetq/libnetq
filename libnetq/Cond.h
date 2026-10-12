/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_COND_H
#define _LIBNETQ_COND_H

#include <libnetq/Basic.h>

#if defined(NQ_OS_WINDOWS)
# include <libnetq/sync/win32/Cond.h>
#elif defined(NQ_OS_UNIX)
# include <libnetq/sync/posix/Cond.h>
#elif defined(NQ_OS_KERNEL)
# include <libnetq/sync/kernel/Cond.h>
#else
# warning There is no implementation for the Cond
# include <libnetq/sync/stub/Cond.h>
#endif

#endif /* _LIBNETQ_COND_H */
