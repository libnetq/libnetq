/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/http/HttpBuilder.h"

#include <libnetq/Assert.h>
#include <libnetq/http/HttpVersion.h>
#include <libnetq/http/HttpHeader.h>
#include <libnetq/http/HttpMethod.h>
#include <libnetq/string/StringUtil.h>

static bool NQHttpBuilder_write(NQHttpBuilder* thiz, const char* characters)
{
  uint16_t newSize = thiz->size;
  NQ_ASSERT(thiz->data[newSize]== '\0');

  for (;;) {
    if (newSize >= sizeof(thiz->data)) {
      return false;
    }
    thiz->data[newSize] = *characters;
    if (*characters == '\0')
      break;
    newSize++;
    characters++;
  }

  thiz->size = newSize;
  return true;
}

bool NQHttpBuilder_init(NQHttpBuilder* thiz)
{
  thiz->data[0] = '\0';
  thiz->size = 0;
  return true;
}

void NQHttpBuilder_finalize(NQHttpBuilder* thiz)
{
}

bool NQHttpBuilder_addRequest(NQHttpBuilder* thiz, const char* method, const char* url, const char* version)
{
  if (!NQHttpBuilder_write(thiz, method))
    return false;

  if (!NQHttpBuilder_write(thiz, " "))
    return false;

  if (!NQHttpBuilder_write(thiz, url))
    return false;

  if (!NQHttpBuilder_write(thiz, " "))
    return false;

  if (!NQHttpBuilder_write(thiz, version))
    return false;

  if (!NQHttpBuilder_write(thiz, NQ_HTTP_CRLF))
    return false;

  return true;
}

bool NQHttpBuilder_addGetRequest(NQHttpBuilder* thiz, const char* url)
{
  if (!NQHttpBuilder_write(thiz, NQ_HTTP_GET " "))
    return false;

  if (!NQHttpBuilder_write(thiz, url))
    return false;

  if (!NQHttpBuilder_write(thiz, " " NQ_HTTP11_VERSION NQ_HTTP_CRLF))
    return false;

  return true;
}

bool NQHttpBuilder_addStatusLine(NQHttpBuilder* thiz, const char* version, const char* code, const char* reason)
{
  if (!NQHttpBuilder_write(thiz, version))
    return false;

  if (!NQHttpBuilder_write(thiz, " "))
    return false;

  if (!NQHttpBuilder_write(thiz, code))
    return false;

  if (!NQHttpBuilder_write(thiz, " "))
    return false;

  if (!NQHttpBuilder_write(thiz, reason))
    return false;

  if (!NQHttpBuilder_write(thiz, NQ_HTTP11_VERSION NQ_HTTP_CRLF))
    return false;

  return true;
}

bool NQHttpBuilder_addStatusCode(NQHttpBuilder* thiz, uint16_t code)
{
  switch (code) {
  case 101:
    return NQHttpBuilder_write(thiz, NQ_HTTP11_VERSION " 101 Switching Protocols" NQ_HTTP_CRLF);
  }

  return false;
}

bool NQHttpBuilder_addHeader(NQHttpBuilder* thiz, const char* name, const char* value)
{
  if (!NQHttpBuilder_write(thiz, name))
    return false;

  if (!NQHttpBuilder_write(thiz, ": "))
    return false;

  if (!NQHttpBuilder_write(thiz, value))
    return false;

  if (!NQHttpBuilder_write(thiz, NQ_HTTP_CRLF))
    return false;

  return true;
}

bool NQHttpBuilder_finish(NQHttpBuilder* thiz)
{
  return NQHttpBuilder_write(thiz, NQ_HTTP_CRLF);
}
