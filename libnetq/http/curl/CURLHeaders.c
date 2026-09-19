/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"

#ifdef WITH_CURL

#include "libnetq/http/curl/CURLHeaders.h"

#include <libnetq/string/String.h>
#include <libnetq/Malloc.h>

void NQCURLHeaders_finalize(NQCURLHeaders* thiz)
{
  struct curl_slist* iter = thiz->impl;
  while (iter) {
    struct curl_slist* next = iter->next;
    NQFree(iter);
    iter = next;
  }
}

bool NQCURLHeaders_add(NQCURLHeaders* thiz, const char* name, const char* value)
{
  size_t klen = NQStrlen(name);
  size_t vlen = NQStrlen(value);
  size_t headerSize = klen + 2 + vlen + 1;

  struct curl_slist* entry = (struct curl_slist*)NQMalloc(sizeof(*entry) + headerSize);
  if (!entry)
    return false;

  char* ptr = (char*)entry + sizeof(*entry);
  entry->data = ptr;
  entry->next = NULL;

  memcpy(ptr, name, klen);
  ptr += klen;
  memcpy(ptr, ": ", 2);
  ptr += 2;
  memcpy(ptr, value, vlen + 1);

  if (thiz->impl == NULL)
    thiz->impl = entry;
  else {
    struct curl_slist* last = thiz->impl;
    while (last->next != NULL)
      last = last->next;
    last->next = entry;
  }

  return true;
}

#endif
