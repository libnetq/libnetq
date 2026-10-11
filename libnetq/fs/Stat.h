/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_STAT_H
#define _LIBNETQ_FS_STAT_H

#include <libnetq/Basic.h>

#if defined(NQ_OS_WINDOWS)
# include <libnetq/fs/win32/Stat.h>
#elif defined(NQ_OS_UNIX)
# include <libnetq/fs/posix/Stat.h>
#elif defined(NQ_OS_KERNEL)
# include <libnetq/fs/kernel/Stat.h>
#else
# include <libnetq/fs/stub/Stat.h>
#endif

#endif /* _LIBNETQ_FS_STAT_H */
