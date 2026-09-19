/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_KERNEL_MKDIR_H
#define _LIBNETQ_FS_KERNEL_MKDIR_H

#include <libnetq/Basic.h>

#ifdef NQ_OS_KERNEL

#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

NQ_EXPORT int NQMkdir(const char* path, mode_t mode);

#ifdef __cplusplus
}
#endif

#endif
#endif /* _LIBNETQ_FS_KERNEL_MKDIR_H */
