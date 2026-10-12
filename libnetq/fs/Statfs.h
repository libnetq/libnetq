/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_STATFS_H
#define _LIBNETQ_FS_STATFS_H

#include <libnetq/Basic.h>

#if defined(NQ_OS_WINDOWS)
# include <libnetq/fs/win32/Statfs.h>
#elif defined(NQ_OS_UNIX)
# include <libnetq/fs/posix/Statfs.h>
#elif defined(NQ_OS_KERNEL)
# include <libnetq/fs/kernel/Statfs.h>
#else
# include <libnetq/fs/stub/Statfs.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

NQ_EXPORT const char* NQStatfsTypeStr(uint64_t type);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LIBNETQ_FS_STATFS_H */
