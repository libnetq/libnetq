/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_PROCESS_PTY_H
#define _LIBNETQ_PROCESS_PTY_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A program running on a pseudo terminal: a pty with a session of its own on
 * Linux and macOS, a pseudo console on Windows 10 1809 or later (older
 * versions get -NQ_ENOTSUPP). What the program writes comes back from
 * NQPtyRead with its terminal's escape sequences, and what NQPtyWrite sends
 * reaches it as typed input.
 *
 * NQPtyRead may block in one thread while another calls NQPtyWrite,
 * NQPtyResize and NQPtyWait; NQPtyClose must wait for all of them.
 */
typedef struct NQPty NQPty;

typedef struct NQPtyOptions {
  const char* program;      /* a path, or a name looked up in PATH */
  const char* const* args;  /* what follows argv[0], NULL-terminated; or NULL */
  const char* cwd;          /* where it starts; NULL to stay here */
  int cols, rows;           /* 80 x 24 when 0 */
} NQPtyOptions;

/* Writes the path of the user's shell into `out`: $SHELL, the password
 * database's or the first of the usual ones that exists; on Windows
 * pwsh.exe or powershell.exe on the path, else %ComSpec%. Returns its
 * length, or a negative error code. */
NQ_EXPORT int NQGetDefaultShell(char* out, size_t size);

/* Starts the program, with this process's environment. Returns 0 and sets
 * *pty, or a negative error code, -NQ_ENOENT and the like when the program
 * cannot be run. */
NQ_EXPORT int NQPtySpawn(NQPty** pty, const NQPtyOptions* options);

/* Waits up to `timeoutMs` for output. Returns the number of bytes read, 0
 * when no program is left on the terminal, -NQ_ETIMEDOUT when nothing came,
 * or another negative error code. */
NQ_EXPORT int NQPtyRead(NQPty* pty, void* buffer, size_t size, int timeoutMs);

/* Sends input without blocking. Returns the number of bytes taken, which is
 * 0 while the program is not reading, or a negative error code. */
NQ_EXPORT int NQPtyWrite(NQPty* pty, const void* data, size_t size);

/* Tells the program the terminal's new size */
NQ_EXPORT int NQPtyResize(NQPty* pty, int cols, int rows);

NQ_EXPORT int64_t NQPtyGetPid(NQPty* pty);

/* Returns 1 and sets *exitCode (128 + the signal for one killed by a signal)
 * when the program has ended, 0 while it runs, or a negative error code. */
NQ_EXPORT int NQPtyWait(NQPty* pty, int* exitCode);

/* Ends the program if it is still running and frees everything */
NQ_EXPORT void NQPtyClose(NQPty* pty);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_PROCESS_PTY_H */
