/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_DIR_H
#define _LIBNETQ_FS_DIR_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct NQDir NQDir;

NQ_EXPORT NQDir* NQDir_open(const char* pathname);
NQ_EXPORT bool NQDir_next(NQDir* dir);
NQ_EXPORT void NQDir_close(NQDir* dir);

NQ_EXPORT const char* NQDir_name(NQDir* dir);
NQ_EXPORT bool NQDir_isFile(NQDir* dir);
NQ_EXPORT bool NQDir_isDirectory(NQDir* dir);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_FS_DIR_H */
