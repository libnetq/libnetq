/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_FS_TEMPFILE_H
#define _LIBNETQ_FS_TEMPFILE_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum NQTempKind {
  NQ_TEMP_FILE = 0,  /* an empty file that nobody else can have made, u+rw */
  NQ_TEMP_DIRECTORY, /* a directory, likewise, u+rwx */
  NQ_TEMP_NAME,      /* nothing: the name was free when looked at, no more */
} NQTempKind;

/*
 * Makes a name no file has: replaces the `count` characters of `path` that
 * end `suffixLength` before its end with random letters and digits, and
 * creates what `kind` says, retrying with other names while it exists.
 * Returns 0, or the negated error code: -NQ_EEXIST when no name was free,
 * -NQ_EINVAL when `count` does not fit. Windows has no permission bits: what
 * is made inherits the access of its directory.
 */
NQ_EXPORT int NQMakeTempPath(char* path, size_t suffixLength, size_t count, NQTempKind kind);

/*
 * Where temporary files go: $TMPDIR if it is set; else what the system says
 * on Windows, /tmp on other systems, and /data/local/tmp on Android, where
 * only the shell may write and an app should set $TMPDIR. Uses '/' on every
 * system. Returns its length, or the negated error code.
 */
NQ_EXPORT int NQGetTempDirectory(char* buffer, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_FS_TEMPFILE_H */
