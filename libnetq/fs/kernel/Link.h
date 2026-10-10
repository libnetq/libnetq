/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_KERNEL_LINK_H
#define _LIBNETQ_FS_KERNEL_LINK_H

#include <libnetq/Basic.h>

#ifdef NQ_OS_KERNEL

#ifdef __cplusplus
extern "C" {
#endif

NQ_EXPORT int NQSymlink(const char* target, const char* linkPath);
NQ_EXPORT int NQLink(const char* existingPath, const char* newPath);

#ifdef __cplusplus
}
#endif

#endif
#endif /* _LIBNETQ_FS_KERNEL_LINK_H */
