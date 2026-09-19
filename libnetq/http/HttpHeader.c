/*
 * MIT License
 *
 * Copyright (c) 2022-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/http/HttpHeader.h"

#include <libnetq/string/StringUtil.h>

static const char* findChar(const char* start, const char* end, char ch)
{
  for (;;) {
    if (end <= start) {
      return NULL;
    }
    if (*start == ch) {
      return start;
    }
    start++;
  }
}

static const char* findCharIfNot(const char* start, const char* end, char ch)
{
  for (;;) {
    if (end <= start) {
      return NULL;
    }
    if (*start != ch) {
      return start;
    }
    start++;
  }
}

bool NQHttpRequestLineParse(const char* data, size_t size, NQHttpRequestLine* result)
{
  const char* start = data;
  const char* end = data + size;

  start = result->method.characters = findCharIfNot(start, end, ' ');
  if (start == NULL) {
    return false;
  }

  start = findChar(start + 1, end, ' ');
  if (start == NULL) {
    return false;
  }

  result->method.length = start - result->method.characters;

  start = result->url.characters = findCharIfNot(start + 1, end, ' ');
  if (start == NULL) {
    return false;
  }

  start = findChar(start + 1, end, ' ');
  if (start == NULL) {
    return false;
  }

  result->url.length = start - result->url.characters;

  start = result->version.characters = findCharIfNot(start + 1, end, ' ');
  if (start == NULL) {
    return false;
  }

  result->version.length = end - start;
  return true;
}

bool NQHttpStatusLineParse(const char* data, size_t size, NQHttpStatusLine* result)
{
  const char* start = data;
  const char* end = data + size;

  start = result->version.characters = findCharIfNot(start, end, ' ');
  if (start == NULL) {
    return false;
  }

  start = findChar(start + 1, end, ' ');
  if (start == NULL) {
    return false;
  }

  result->version.length = start - result->version.characters;

  start = result->code.characters = findCharIfNot(start + 1, end, ' ');
  if (start == NULL) {
    return false;
  }

  start = findChar(start + 1, end, ' ');
  if (start == NULL) {
    return false;
  }

  result->code.length = start - result->code.characters;

  start = result->reason.characters = findCharIfNot(start + 1, end, ' ');
  if (start == NULL) {
    return false;
  }

  result->reason.length = end - start;
  return true;
}

bool NQHttpHeaderLineParse(const char* data, size_t size, NQHttpHeaderLine* result)
{
  const char* start = data;
  const char* end = data + size;

  start = result->name.characters = findCharIfNot(start, end, ' ');
  if (start == NULL) {
    return false;
  }

  start = findChar(start + 1, end, ':');
  if (start == NULL) {
    return false;
  }

  result->name.length = start - result->name.characters;

  start = result->value.characters = findCharIfNot(start + 1, end, ' ');
  if (start == NULL) {
    return false;
  }

  result->value.length = end - start;
  return true;
}

bool NQHttpHeaderValueParse(const char* data, size_t size, NQHttpHeaderValue* result)
{
  const char* end = data + size;

  for (;;) {
    if (end <= data) {
      return false;
    }
    if (*data != ' ') {
      break;
    }
    data++;
  }

  size_t klen = 0;
  const char* key = data;
  for (;;) {
    if (end <= data) {
      return false;
    }
    char ch = *data++;
    if (ch == '=') {
      klen = data - key - 1;
      break;
    }
    if (ch != ' ') {
      continue;
    }
    klen = data - key - 1;
    for (;;) {
      if (end <= data) {
        return false;
      }
      ch = *data++;
      if (ch == '=') {
        break;
      }
      if (ch != ' ') {
        return false;
      }
    }
    break;
  }

  for (;;) {
    if (end <= data) {
      return false;
    }
    if (*data != ' ') {
      break;
    }
    data++;
  }

  if (*data != '"') {
    return false;
  }

  if (end <= ++data) {
    return false;
  }

  size_t vlen = 0;
  const char* val = data;
  for (;;) {
    if (end <= data) {
      return false;
    }
    if (data[vlen] == '"') {
      data += vlen + 1;
      break;
    }
    vlen++;
  }

  while (data < end) {
    if (*data != ' ') {
      return false;
    }
  }

  if (result != NULL) {
    result->key.characters = key;
    result->key.length = klen;
    result->value.characters = val;
    result->value.length = vlen;
  }

  return true;
}

bool NQHttpFormDataParse(const char* data, size_t size, NQHttpFormData* result)
{
  size_t count = 0;
  const char* curr = data;
  const char* end = curr + size;

  if (result != NULL) {
    memset(result, 0, sizeof(*result));
  }

  while (curr < end) {
    while (curr < end && *curr == ' ') {
      curr++;
    }
    const char* data = curr;
    while (curr < end) {
      if (*curr++ == ';')
        break;
    }
    size_t size = curr - data - 1;
    while (size != 0 && data[size - 1] == ' ') {
      size--;
    }
    if (count == 0) {
      if (size != 9 || memcmp(data, "form-data", 9) != 0) {
        return false;
      }
      count++;
      continue;
    }
    NQHttpHeaderValue kv;
    if (NQHttpHeaderValueParse(data, size, &kv)) {
      if (kv.key.length == 4 && memcmp("name", kv.key.characters, kv.key.length) == 0) {
        if(result != NULL)
          result->name = kv.value;
      }
      else if (kv.key.length == 8 && memcmp("filename", kv.key.characters, kv.key.length) == 0) {
        if(result != NULL)
          result->filename = kv.value;
      }
    }
    count++;
    data += size;
  }

  return count > 0;
}

const char* NQHttpGetContentBoundary(const char* contentType)
{
  while (*contentType == ' ')
    contentType++;

  const char multipartFormData[] = "multipart/form-data;";
  if (memcmp(contentType, multipartFormData, sizeof(multipartFormData) - 1) != 0)
    return NULL;
  contentType += sizeof(multipartFormData) - 1;

  while (*contentType == ' ')
    contentType++;

  const char boundary[] = "boundary=";
  if (memcmp(contentType, boundary, sizeof(boundary) - 1) != 0)
    return NULL;
  contentType += sizeof(boundary) - 1;

  return contentType;
}
