/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_LINK_H
#define _LIBNETQ_FS_LINK_H

#include <libnetq/Basic.h>

/*
 * int NQSymlink(const char* target, const char* linkPath);
 *   Creates a symbolic link at linkPath pointing to target.
 *
 * int NQLink(const char* existingPath, const char* newPath);
 *   Creates a hard link newPath to existingPath.
 *
 * Both return 0 on success or a negative NQ error code.
 */

#if defined(NQ_OS_WINDOWS)
# include <libnetq/fs/win32/Link.h>
#elif defined(NQ_OS_UNIX)
# include <libnetq/fs/posix/Link.h>
#elif defined(NQ_OS_KERNEL)
# include <libnetq/fs/kernel/Link.h>
#else
# include <libnetq/fs/stub/Link.h>
#endif

#endif /* _LIBNETQ_FS_LINK_H */
