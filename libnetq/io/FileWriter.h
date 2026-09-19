/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_IO_FILEWRITER_H
#define _LIBNETQ_IO_FILEWRITER_H

#include <libnetq/io/IOWriter.h>
#include <libnetq/fs/FileHandle.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQFileWriter NQFileWriter;
struct NQFileWriter {
  NQIOWriter base;
  NQFileHandle handle;
};

NQ_EXPORT NQFileWriter* NQFileWriter_create(const char* filename);
NQ_EXPORT void NQFileWriter_destroy(NQFileWriter*);

NQ_EXPORT bool NQFileWriter_init(NQFileWriter*, const char* filename);
NQ_EXPORT void NQFileWriter_finalize(NQFileWriter*);

NQ_EXPORT int NQFileWriter_write(NQFileWriter*, const void* data, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_IO_FILEWRITER_H */
