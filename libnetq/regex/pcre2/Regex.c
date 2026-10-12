/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_CLASS_NAME "NQRegex"
#define NQ_LOG_TAG NQ_CLASS_NAME

#include "config.h"
#include "libnetq/regex/Regex.h"

#ifdef NQCONFIG_USE_PCRE2_REGEX

#include <libnetq/String.h>
#include <libnetq/Log.h>
#include <libnetq/Assert.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

const char* NQRegex_package(void)
{
  return "pcre2";
}

const char* NQRegex_version(void)
{
  return NQ_STRINGIZE_OF(PCRE2_MAJOR) "." NQ_STRINGIZE_OF(PCRE2_MINOR);
}

NQRegex* NQRegex_create(const char* pattern, int flags)
{
  return NQRegex_create2(pattern, PCRE2_ZERO_TERMINATED, flags);
}

NQRegex* NQRegex_create2(const char* pattern, size_t length, int flags)
{
  int errorCode;
  size_t errorOffset;
  char errorBuffer[128];

  int options = 0;
  if (flags & NQ_REGEX_CASELESS)
    options |= PCRE2_CASELESS;
  if (flags & NQ_REGEX_MULTILINE)
    options |= PCRE2_MULTILINE;

  NQRegex* thiz = (NQRegex*)pcre2_compile(pattern, length, options, &errorCode, &errorOffset, NULL);
  if (thiz == NULL) {
    pcre2_get_error_message(errorCode, errorBuffer, sizeof(errorBuffer));
    NQ_LOGE("pcre2_compile failed at offset %llu: %s", (unsigned long long)errorOffset, errorBuffer);
  }

  return thiz;
}

void NQRegex_release(NQRegex* thiz)
{
  pcre2_code* code = (pcre2_code*)thiz;
  pcre2_code_free(code);
}

NQRegexMatch* NQRegexMatch_create(NQRegex* regex)
{
  pcre2_code* code = (pcre2_code*)regex;
  pcre2_match_data* data = pcre2_match_data_create_from_pattern(code, NULL);
  return (NQRegexMatch*)data;
}

NQRegexMatch* NQRegexMatch_create2(size_t size)
{
  pcre2_match_data* data = pcre2_match_data_create((uint32_t)size, NULL);
  return (NQRegexMatch*)data;
}

void NQRegexMatch_release(NQRegexMatch* match)
{
  pcre2_match_data* data = (pcre2_match_data*)match;
  pcre2_match_data_free(data);
}

size_t NQRegexMatch_count(const NQRegexMatch* thiz)
{
  pcre2_match_data* data = (pcre2_match_data*)thiz;
  return pcre2_get_ovector_count(data);
}

bool NQRegexMatch_substrAt(const NQRegexMatch* thiz, size_t index, size_t* offset, size_t* length)
{
  pcre2_match_data* data = (pcre2_match_data*)thiz;

  NQ_ASSERT(index < pcre2_get_ovector_count(data));
  NQ_ASSERT(offset && length);

  size_t* ovector = pcre2_get_ovector_pointer(data);

  size_t startOffset = ovector[2 * index];
  size_t endOffset = ovector[2 * index + 1];

  if (startOffset == PCRE2_UNSET || endOffset == PCRE2_UNSET)
    return false;

  *offset = startOffset;
  *length = endOffset - startOffset;

  return true;
}

bool NQRegex_exec(NQRegex* thiz, const char* str, NQRegexMatch* match, int flags)
{
  return NQRegex_exec2(thiz, str, PCRE2_ZERO_TERMINATED, match, flags);
}

bool NQRegex_exec2(NQRegex* thiz, const char* str, size_t len, NQRegexMatch* match, int flags)
{
  pcre2_code* code = (pcre2_code*)thiz;
  pcre2_match_data* data = (pcre2_match_data*)match;

  int ret = pcre2_match(code, str, len, 0, 0, data, NULL);
  if (ret < 0) {
    if (ret != PCRE2_ERROR_NOMATCH)
      NQ_LOGE("pcre2_match failed %i", ret);
    return false;
  }

  return true;
}

int NQRegex_indexOf(const NQRegex* thiz, const char* name)
{
  pcre2_code* code = (pcre2_code*)thiz;

  int ret = pcre2_substring_number_from_name(code, name);
  if (ret < 0) {
    NQ_LOGE("pcre2_substring_number_from_name failed %i", ret);
    return -1;
  }

  return ret;
}

#endif
