/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/http/HttpMultiPartBuilder.h"

#include <libnetq/Limits.h>
#include <libnetq/Malloc.h>
#include <libnetq/string/String.h>
#include <libnetq/http/HttpHeader.h>
#include <libnetq/http/HttpMultiPartParser.h>

#define kContentTypeMax (32)
#define kFileNameMax (64)

enum {
  kInitState,
  kBeginState,
  kBoundaryState,
  kContentTypeState,
  kContentDispositionState,
  kFieldNameState,
  kFieldValueState,
  kBodyPartState,
  kCompliteState,
  kDoneState,
  kErrorState,
};

struct NQHttpFormDataEntry {
  struct NQHttpFormDataEntry* next;
  NQHttpFormDataBuffer formData;
  int flags;
  uint32_t maxBufferSize;
  bool hasFormData;
  uint8_t nameLength;
  char filename[kFileNameMax];
  char contentType[kContentTypeMax];
};

struct NQHttpMultiPartBuilder {
  NQHTTPMultiPartParser* parser;
  struct NQHttpFormDataEntry* first;
  struct NQHttpFormDataEntry* curr;
  uint8_t state;
  bool hasContentType;
  char contentType[kContentTypeMax];
};

static inline bool onMultiPartBegin(NQHttpMultiPartBuilder* thiz, const char* data, size_t size)
{
  if (thiz->state == kInitState) {
    thiz->state = kBeginState;
    return true;
  }

  return false;
}

static inline bool onMultiPartBoundary(NQHttpMultiPartBuilder* thiz, const char* data, size_t size)
{
  if (thiz->state == kBeginState) {
    thiz->state = kBoundaryState;
    return true;
  }

  if (thiz->state == kBodyPartState) {
    thiz->state = kBoundaryState;
    thiz->curr = NULL;
    thiz->hasContentType = false;
    return true;
  }

  return false;
}

static inline bool onMultiPartFieldName(NQHttpMultiPartBuilder* thiz, const char* data, size_t size)
{
  if (thiz->state == kBoundaryState || thiz->state == kFieldValueState) {
    if (NQ_CSTR_LENGTH(NQHTTP_HEADER_CONTENT_DISPOSITION) == size && memcmp(NQHTTP_HEADER_CONTENT_DISPOSITION, data, size) == 0)
      thiz->state = kContentDispositionState;
    else if (NQ_CSTR_LENGTH(NQHTTP_HEADER_CONTENT_TYPE) == size && memcmp(NQHTTP_HEADER_CONTENT_TYPE, data, size) == 0)
      thiz->state = kContentTypeState;
    else
      thiz->state = kFieldNameState;
    return true;
  }

  return false;
}

static inline bool onMultiPartFieldValue(NQHttpMultiPartBuilder* thiz, const char* data, size_t size)
{
  NQHttpFormData formData;
  struct NQHttpFormDataEntry* entry;

  switch (thiz->state) {
  case kContentDispositionState:
    if (thiz->curr != NULL)
      return false;

    if (!NQHttpFormDataParse(data, size, &formData))
      return false;

    entry = thiz->first;
    while (entry != NULL) {
      if (entry->nameLength == formData.name.length && memcmp(entry->formData.name, formData.name.characters, formData.name.length) == 0) {
        if (entry->flags & kNQHttpFormDataWithFilename) {
          if (formData.filename.length == 0 || formData.filename.length >= kFileNameMax)
            return false;
          memcpy(entry->filename, formData.filename.characters, formData.filename.length);
          entry->filename[formData.filename.length] = 0;
          entry->formData.filename = entry->filename;
        }
        if (thiz->hasContentType) {
          memcpy(entry->contentType, thiz->contentType, kContentTypeMax);
          entry->formData.contentType = entry->contentType;
        }
        thiz->curr = entry;
        break;
      }
      entry = entry->next;
    }

    break;

  case kContentTypeState:
    if (size >= kContentTypeMax)
      return false;

    if (thiz->curr != NULL) {
      memcpy(thiz->curr->contentType, data, size);
      thiz->curr->contentType[size] = 0;
      thiz->curr->formData.contentType = thiz->curr->contentType;
    }
    else {
      memcpy(thiz->contentType, data, size);
      thiz->contentType[size] = 0;
      thiz->hasContentType = true;
    }

    break;

  case kFieldNameState:
    break;

  default:
    return false;
  }

  thiz->state = kFieldValueState;
  return true;
}

static inline bool onMultiPartBodyPart(NQHttpMultiPartBuilder* thiz, const char* data, size_t size)
{
  switch (thiz->state) {
  case kFieldValueState:
    if (thiz->curr != NULL) {
      if (thiz->curr->flags & kNQHttpFormDataWithContentType) {
        if (thiz->curr->formData.contentType == NULL)
          return false;
      }
      thiz->curr->hasFormData = true;
    }
    thiz->state = kBodyPartState;
    break;

  case kBodyPartState:
    break;

  default:
    return false;
  }

  if (thiz->curr != NULL) {
    size_t oldSize = NQByteBuffer_size(&thiz->curr->formData.buffer);
    size_t newSize = oldSize + size;
    if (newSize < oldSize)
      return false;
    if (thiz->curr->maxBufferSize != 0 && thiz->curr->maxBufferSize < newSize)
      return false;
    return NQByteBuffer_append(&thiz->curr->formData.buffer, (const uint8_t*)data, size);
  }

  return true;
}

static inline bool onMultiPartFinish(NQHttpMultiPartBuilder* thiz, const char* data, size_t size)
{
  if (thiz->state == kBoundaryState) {
    thiz->state = kCompliteState;
    return true;
  }

  return false;
}

static bool onMultiPartParser(void* userdata, NQHTTPMultiPartType type, const char* data, size_t size)
{
  NQHttpMultiPartBuilder* thiz = (NQHttpMultiPartBuilder*)userdata;

  switch (type) {
  case kNQHTTPMultiPartBegin:
    return onMultiPartBegin(thiz, data, size);

  case kNQHTTPMultiPartBoundary:
    return onMultiPartBoundary(thiz, data, size);

  case kNQHTTPMultiPartFieldName:
    return onMultiPartFieldName(thiz, data, size);

  case kNQHTTPMultiPartFieldValue:
    return onMultiPartFieldValue(thiz, data, size);

  case kNQHTTPMultiPartBodyPart:
    return onMultiPartBodyPart(thiz, data, size);

  case kNQHTTPMultiPartFinish:
    return onMultiPartFinish(thiz, data, size);
  }

  return false;
}

NQHttpMultiPartBuilder* NQHttpMultiPartBuilder_create(const char* boundary)
{
  NQHttpMultiPartBuilder* thiz = (NQHttpMultiPartBuilder*)NQMalloc(sizeof(NQHttpMultiPartBuilder));
  if (thiz == NULL)
    return NULL;

  thiz->parser = NQHTTPMultiPartParser_create(boundary, &onMultiPartParser, thiz);
  if (thiz->parser == NULL) {
    NQFree(thiz);
    return NULL;
  }

  thiz->first = NULL;
  thiz->curr = NULL;
  thiz->state = kInitState;
  thiz->hasContentType = false;

  return thiz;
}

void NQHttpMultiPartBuilder_destroy(NQHttpMultiPartBuilder* thiz)
{
  struct NQHttpFormDataEntry* iter = thiz->first;
  while (iter != NULL) {
    struct NQHttpFormDataEntry* entry = iter;
    iter = iter->next;
    NQByteBuffer_finalize(&entry->formData.buffer);
    NQFree(entry);
  }
  NQFree(thiz);
}

bool NQHttpMultiPartBuilder_enableFormData(NQHttpMultiPartBuilder* thiz, const char* name, int flags, uint32_t maxBufferSize)
{
  if (thiz->state != kInitState)
    return false;

  size_t nameLength = strlen(name);
  if (nameLength >= NQ_UINT8_MAX)
    return false;

  struct NQHttpFormDataEntry* entry = (struct NQHttpFormDataEntry*)NQMalloc(sizeof(struct NQHttpFormDataEntry) + nameLength + 1);
  if (entry == NULL)
    return false;

  entry->formData.name = (char*)entry + sizeof(struct NQHttpFormDataEntry);
  entry->nameLength = (uint8_t)nameLength;
  memcpy(entry->formData.name, name, nameLength + 1);

  entry->formData.filename = NULL;
  entry->formData.contentType = NULL;

  NQByteBuffer_init(&entry->formData.buffer);

  entry->next = NULL;
  entry->flags = flags;
  entry->hasFormData = false;
  entry->maxBufferSize = maxBufferSize;

  entry->next = thiz->first;
  thiz->first = entry;

  return true;
}

bool NQHttpMultiPartBuilder_append(NQHttpMultiPartBuilder* thiz, const uint8_t* data, size_t size)
{
  if (thiz->state != kErrorState) {
    if (NQHTTPMultiPartParser_append(thiz->parser, (const char*)data, size))
      return true;
    thiz->state = kErrorState;
  }
  return false;
}

bool NQHttpMultiPartBuilder_finish(NQHttpMultiPartBuilder* thiz)
{
  if (thiz->state != kCompliteState) {
    thiz->state = kErrorState;
    return false;
  }

  if (!NQHTTPMultiPartParser_finish(thiz->parser)) {
    thiz->state = kErrorState;
    return false;
  }

  struct NQHttpFormDataEntry* entry = thiz->first;
  while (entry != NULL) {
    if (entry->flags & kNQHttpFormDataMandatory) {
      if (!entry->hasFormData) {
        thiz->state = kErrorState;
        return false;
      }
    }
    entry = entry->next;
  }

  thiz->state = kDoneState;
  return true;
}

NQHttpFormDataBuffer* NQHttpMultiPartBuilder_formData(NQHttpMultiPartBuilder* thiz, const char* name)
{
  if (thiz->state != kDoneState)
    return false;

  struct NQHttpFormDataEntry* entry = thiz->first;
  while (entry != NULL) {
    if (!strcmp(entry->formData.name, name))
      return entry->hasFormData ? &entry->formData : NULL;
    entry = entry->next;
  }

  return NULL;
}
