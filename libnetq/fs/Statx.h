/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_STATX_H
#define _LIBNETQ_FS_STATX_H

#include <libnetq/Basic.h>

#if defined(NQ_OS_WINDOWS)
# include <libnetq/fs/win32/Statx.h>
#elif defined(NQ_OS_UNIX)
# include <libnetq/fs/posix/Statx.h>
#elif defined(NQ_OS_KERNEL)
# include <libnetq/fs/kernel/Statx.h>
#else
# include <libnetq/fs/stub/Statx.h>
#endif

#endif /* _LIBNETQ_FS_STATX_H */
