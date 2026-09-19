/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_STRING_STRINGARRAY_H
#define _LIBNETQ_STRING_STRINGARRAY_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQStringArray8 NQStringArray8;
struct NQStringArray8 {
  uint8_t length;
  char characters[1];
};

NQ_EXPORT NQStringArray8* NQStringArray8_alloc(size_t length);
NQ_EXPORT NQStringArray8* NQStringArray8_create(const char* characters);
NQ_EXPORT NQStringArray8* NQStringArray8_create2(const char* characters, size_t length);
NQ_EXPORT void NQStringArray8_destroy(NQStringArray8*);
NQ_EXPORT void NQStringArray8_shrink(NQStringArray8*, size_t newLength);

#define NQStringArray8_characters(thiz) (thiz)->characters

static inline size_t NQStringArray8_length(const NQStringArray8* thiz)
{
  return thiz->length;
}

static inline bool NQStringArray8_isEmpty(const NQStringArray8* thiz)
{
  return thiz->length == 0;
}

typedef struct NQStringArray16 NQStringArray16;
struct NQStringArray16 {
  uint16_t length;
  char characters[1];
};

NQ_EXPORT NQStringArray16* NQStringArray16_alloc(size_t length);
NQ_EXPORT NQStringArray16* NQStringArray16_create(const char* characters);
NQ_EXPORT NQStringArray16* NQStringArray16_create2(const char* characters, size_t length);
NQ_EXPORT void NQStringArray16_destroy(NQStringArray16*);
NQ_EXPORT void NQStringArray16_shrink(NQStringArray16*, size_t newLength);

#define NQStringArray16_characters(thiz) (thiz)->characters

static inline size_t NQStringArray16_length(const NQStringArray16* thiz)
{
  return thiz->length;
}

static inline bool NQStringArray16_isEmpty(const NQStringArray16* thiz)
{
  return thiz->length == 0;
}

typedef struct NQStringArray NQStringArray;
struct NQStringArray {
  uint32_t length;
  char characters[1];
};

NQ_EXPORT NQStringArray* NQStringArray_alloc(size_t length);
NQ_EXPORT NQStringArray* NQStringArray_create(const char* characters);
NQ_EXPORT NQStringArray* NQStringArray_create2(const char* characters, size_t length);
NQ_EXPORT void NQStringArray_destroy(NQStringArray*);
NQ_EXPORT void NQStringArray_shrink(NQStringArray*, size_t newLength);

#define NQStringArraye_characters(thiz) (thiz)->characters

static inline size_t NQStringArray_length(const NQStringArray* thiz)
{
  return thiz->length;
}

static inline bool NQStringArray_isEmpty(const NQStringArray* thiz)
{
  return thiz->length == 0;
}

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_STRING_STRINGARRAY_H */
