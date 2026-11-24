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

typedef struct NQDirImpl NQDir;

NQ_EXPORT int NQDir_open(const char* pathname, NQDir** result);
NQ_EXPORT void NQDir_close(NQDir*);

NQ_EXPORT bool NQDir_first(NQDir*);
NQ_EXPORT bool NQDir_next(NQDir*);

NQ_EXPORT const char* NQDir_name(NQDir*);
NQ_EXPORT bool NQDir_isFile(NQDir*);
NQ_EXPORT bool NQDir_isDirectory(NQDir*);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_FS_DIR_H */
