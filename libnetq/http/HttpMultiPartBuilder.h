/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_HTTP_HTTPMULTIPARTBUILDER_H
#define _LIBNETQ_HTTP_HTTPMULTIPARTBUILDER_H

#include <libnetq/ByteBuffer.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQHttpMultiPartBuilder NQHttpMultiPartBuilder;
typedef struct NQHttpFormDataBuffer NQHttpFormDataBuffer;

enum {
  kNQHttpFormDataOptional = (0),
  kNQHttpFormDataMandatory = (1 << 0),
  kNQHttpFormDataWithFilename = (1 << 1),
  kNQHttpFormDataWithContentType = (1 << 2),
};

struct NQHttpFormDataBuffer {
  char* name;
  char* filename;
  char* contentType;
  NQByteBuffer buffer;
};

NQ_EXPORT NQHttpMultiPartBuilder* NQHttpMultiPartBuilder_create(const char* boundary);
NQ_EXPORT void NQHttpMultiPartBuilder_destroy(NQHttpMultiPartBuilder*);
NQ_EXPORT bool NQHttpMultiPartBuilder_enableFormData(NQHttpMultiPartBuilder*, const char* name, int flags, uint32_t maxBufferSize);
NQ_EXPORT bool NQHttpMultiPartBuilder_append(NQHttpMultiPartBuilder*, const uint8_t* data, size_t size);
NQ_EXPORT bool NQHttpMultiPartBuilder_finish(NQHttpMultiPartBuilder*);
NQ_EXPORT NQHttpFormDataBuffer* NQHttpMultiPartBuilder_formData(NQHttpMultiPartBuilder*, const char* name);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_HTTP_HTTPMULTIPARTBUILDER_H */
