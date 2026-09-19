/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_MKDIR_H
#define _LIBNETQ_FS_MKDIR_H

#include <libnetq/Basic.h>

#if defined(NQ_OS_WINDOWS)
# include <libnetq/fs/win32/Mkdir.h>
#elif defined(NQ_OS_UNIX)
# include <libnetq/fs/posix/Mkdir.h>
#elif defined(NQ_OS_KERNEL)
# include <libnetq/fs/kernel/Mkdir.h>
#else
# include <libnetq/fs/stub/Mkdir.h>
#endif

#endif /* _LIBNETQ_FS_MKDIR_H */
