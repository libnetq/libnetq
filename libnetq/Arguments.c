/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_CLASS_NAME "NQArguments"
#define NQ_LOG_TAG NQ_CLASS_NAME

#include "config.h"
#include "libnetq/Arguments.h"

#include <libnetq/string/String.h>
#include <libnetq/string/Strtox.h>
#include <libnetq/PrimitiveType.h>
#include <libnetq/Limits.h>
#include <libnetq/Malloc.h>
#include <libnetq/UTF.h>
#include <libnetq/Log.h>
#include <libnetq/CType.h>
#include <libnetq/Assert.h>

#define NQ_ARG_MAX 4096

NQ_STATIC_ASSERT(sizeof(NQArguments) % sizeof(void*) == 0, "NQArguments is not aligned");

static char* s_argvEmpty[] = { NULL };
static NQArguments s_argumentsEmpty =
{
  .argc = 0,
  .argv = s_argvEmpty,
};

#ifdef NQ_OS_WINDOWS
static HINSTANCE s_hInstance = NULL;
static HINSTANCE s_hPrevInstance = NULL;
static PSTR s_lpCmdLine = NULL;
static PWSTR s_lpCmdLineW = NULL;
static INT s_iCmdShow = 0;
#endif

static NQArguments* s_arguments = &s_argumentsEmpty;

static const char* skipKey(const char* arg, const char* key)
{
  if (*arg != '-')
    return NULL;
  arg++;

  if (*arg != '-')
    return NULL;
  arg++;

  size_t len = strlen(key);
  if (memcmp(arg, key, len) != 0)
    return NULL;
  arg += len;

  return arg;
}

const char* NQArgGetStringOr(const char* arg, const char* key, const char* defval)
{
  arg = skipKey(arg, key);
  if (arg == NULL)
    return defval;

  if (*arg != '=')
    return defval;
  arg++;

  return arg;
}

int32_t NQArgGetInt32Or(const char* arg, const char* key, int32_t defval)
{
  int32_t result;
  if (NQArgGetInt32(arg, key, &result))
    return result;
  return defval;
}

int64_t NQArgGetInt64Or(const char* arg, const char* key, int64_t defval)
{
  int64_t result;
  if (NQArgGetInt64(arg, key, &result))
    return result;
  return defval;
}

uint32_t NQArgGetUint32Or(const char* arg, const char* key, uint32_t defval)
{
  uint32_t result;
  if (NQArgGetUint32(arg, key, &result))
    return result;
  return defval;
}

uint64_t NQArgGetUint64Or(const char* arg, const char* key, uint64_t defval)
{
  uint64_t result;
  if (NQArgGetUint64(arg, key, &result))
    return result;
  return defval;
}

bool NQArgHasKey(const char* arg, const char* key)
{
  arg = skipKey(arg, key);
  if (arg == NULL)
    return false;

  return (*arg == '=' || *arg == '\0');
}

bool NQArgHasKeyOnly(const char* arg, const char* key)
{
  arg = skipKey(arg, key);
  if (arg == NULL)
    return false;

  return (*arg == '\0');
}

bool NQArgGetBool(const char* arg, const char* key, bool* result)
{
  arg = skipKey(arg, key);
  if (arg == NULL)
    return false;

  if (*arg == '\0') {
    if (result) *result = true;
    return true;
  }

  if (*arg != '=')
    return false;
  arg++;

  if (NQStrIsEnabled(arg)) {
    if (result) *result = false;
    return true;
  }

  if (NQStrIsDisabled(arg)) {
    if (result) *result = false;
    return true;
  }

  return false;
}

bool NQArgGetInt32(const char* arg, const char* key, int32_t* result)
{
  int64_t num;
  if (!NQArgGetInt64(arg, key, &num))
    return false;

  if (num < NQ_INT32_MIN || NQ_INT32_MAX < num)
    return false;

  if (result)
    *result = (int32_t)num;

  return true;
}

bool NQArgGetInt64(const char* arg, const char* key, int64_t* result)
{
  const char* val = NQArgGetStringOr(arg, key, NULL);
  if (val == NULL)
    return false;

  char* end;
  long long num = NQSimpleStrtoll(val, &end, 10);
  if (*end != '\0')
    return false;

  if (result)
    *result = (int64_t)num;

  return true;
}

bool NQArgGetUint32(const char* arg, const char* key, uint32_t* result)
{
  uint64_t num;
  if (!NQArgGetUint64(arg, key, &num))
    return false;

  if (NQ_UINT32_MAX < num)
    return false;

  if (result)
    *result = (uint32_t)num;

  return true;
}


bool NQArgGetUint64(const char* arg, const char* key, uint64_t* result)
{
  const char* val = NQArgGetStringOr(arg, key, NULL);
  if (val == NULL)
    return false;

  char* end;
  unsigned long long num = NQSimpleStrtoull(val, &end, 10);
  if (*end != '\0')
    return false;

  if (result)
    *result = (uint64_t)num;

  return true;
}

bool NQArgGetBoolOr(const char* arg, const char* key, bool defval)
{
  bool result;
  if (NQArgGetBool(arg, key, &result))
    return result;
  return defval;
}

static size_t parseCommandLineUTF16(const uint16_t* str, char* buffer, char** argv, int* argc)
{
  const uint16_t* start = str;
  uint8_t* p = (uint8_t*)buffer;

  int n = 0;
  size_t size = 0;
  bool isQm = false;
  bool isSlash = false;
  bool isText = false;
  char* text = NULL;
  size_t sz;

  for (;;) {
    NQUChar ch = NQUCharNext16(start, NULL, (const uint16_t**)&start);
    if (ch == '\0') {
      if (isSlash) {
        sz = (p == NULL) ? NQUCharSize8('\\') : NQUCharPush8(p, NULL, &p, '\\');
        if (sz == 0) {
          size = 0;
          break;
        }
        size += sz;
      }
      if (isText) {
        if (p != NULL)
          *p++ = '\0';
        if (argv != NULL)
          argv[n] = text;
        n++;
        size++;
      }
      break;
    }

    if (!NQIsUChar(ch))
      break;

    if (isQm) {
       if (!isSlash && ch == '\\')
         isSlash = true;
       else if (!isSlash && ch == '\"') {
        isQm = false;
        isSlash = false;
      }
      else {
        if (isSlash) {
          isSlash = false;
          if (ch != '\"') {
            sz = (p == NULL) ? NQUCharSize8('\\') : NQUCharPush8(p, NULL, &p, '\\');
            if (sz == 0) {
              size = 0;
              break;
            }
            size += sz;
          }
        }
        sz = (p == NULL) ? NQUCharSize8(ch) : NQUCharPush8(p, NULL, &p, ch);
        if (sz == 0) {
          size = 0;
          break;
        }
        size += sz;
      }
      continue;
    }

    if (ch == '\"') {
      isQm = true;
      if (!isText) {
        text = (char*)p;
        isText = true;
      }
      continue;
    }

    if (NQIsSpace(ch)) {
      if (isText) {
        if (p != NULL)
          *p++ = '\0';
        if (argv != NULL)
          argv[n] = text;
        n++;
        size++;
        isText = false;
      }
      continue;
    }

    if (!isText) {
      text = (char*)p;
      isText = true;
    }

    sz = (p == NULL) ? NQUCharSize8(ch) : NQUCharPush8(p, NULL, &p, ch);
    if (sz == 0) {
      size = 0;
      break;
    }
    size += sz;
  }

  if (argc != NULL)
    *argc = (size != 0) ? n : 0;

  return size;
}

NQArguments* NQArguments_create(int argc, const char* argv[])
{
  // TODO: For windows platform convert locale to utf8
  int i;

  size_t classSize = NQ_ALIGN_UP(sizeof(NQArguments), sizeof(char*));
  size_t argvSize = (size_t)(argc + 1) * sizeof(char*);

  size_t charSize = 0;
  for (i = 0; i < argc; i++)
    charSize +=  strlen(argv[i]) + 1;

  NQArguments* thiz = (NQArguments*)NQMalloc(classSize + argvSize + charSize);
  if (thiz == NULL)
    return NULL;

  char** pargv = (char**)(void*)((char*)thiz + classSize);
  char* pchar = (char*)((char*)pargv + argvSize);

  char** pargvStart = pargv;
  char* pcharStart = pchar;
  for (i = 0; i < argc; i++) {
    const char* arg = argv[i];
    *pargvStart++ = pcharStart;
    for (;;) {
      *pcharStart = *arg;
      if (*pcharStart++ == '\0')
        break;
      arg++;
    }
  }
  *pargvStart++ = NULL;

  NQ_ASSERT((char*)pargvStart == (char*)pchar);
  NQ_ASSERT(((char*)thiz + classSize + argvSize + charSize) == pcharStart);

  thiz->argc = argc;
  thiz->argv = pargv;

  return thiz;
}

struct ParseInfo {
  int argc;
};

static NQ_ALWAYS_INLINE size_t argumentsParase(NQArguments* thiz, const char* str, size_t len, struct ParseInfo* info)
{
  char* ptr;

  if (thiz) {
    ptr = (char*)thiz + sizeof(*thiz);
    thiz->argv = (char**)ptr;
    ptr += sizeof(char*) * (info->argc + 1);
    thiz->argv[0] = ptr;
  }

  size_t bytes = sizeof(*thiz);

  char qch;
  bool escape = false;
  unsigned quoted = 0;
  unsigned length = 0;
  int argc = 0;

  for (size_t i = 0; i < len; i++) {
    char ch = str[i];

    if (escape) {
      if (thiz)
        *ptr++ = ch;
      length++;
      escape = false;
      continue;
    }

    if (ch == '\\') {
      escape = true;
      continue;
    }

    if (quoted % 2) {
      if (ch == qch)
        quoted++;
      else {
        if (thiz)
          *ptr++ = ch;
        length++;
      }
      continue;
    }

    if (ch == '"' || ch == '\'') {
      qch = ch;
      quoted++;
      continue;
    }

    if (NQIsSpace(ch)) {
      if (quoted == 0 && length == 0)
        continue;
      argc++;
      if (thiz) {
        *ptr++ = '\0';
        thiz->argv[argc] = ptr;
      }
      bytes += length + 1;
      length = 0;
      continue;
    }

    if (thiz)
      *ptr++ = ch;
    length++;
  }

  if (escape) {
    NQ_LOGW("Incomplete escape");
    if (thiz)
      *ptr++ = '\\';
    length++;
  }

  if (quoted % 2) {
    NQ_LOGW("Unmatched quote");
  }

  if (quoted || length) {
    argc++;
    if (thiz)
      *ptr++ = '\0';
    bytes += length + 1;
  }

  bytes += sizeof(char*) * (argc + 1);

  if (thiz) {
    thiz->argc = argc;
    thiz->argv[argc] = NULL;
    NQ_ASSERT(info->argc == argc);
    NQ_ASSERT((char*)thiz + bytes == ptr);
  }
  else {
    info->argc = argc;
  }

  return bytes;
}

NQArguments* NQArguments_parse(const char* s)
{
  return NQArguments_parse2(s, strlen(s));
}

NQArguments* NQArguments_parse2(const char* str, size_t len)
{
  struct ParseInfo info;

  size_t sizeInBytes = argumentsParase(NULL, str, len, &info);
  if (info.argc == 0)
    return &s_argumentsEmpty;

  NQArguments* thiz = (NQArguments*)NQMalloc(sizeInBytes);
  if (thiz == NULL)
    return NULL;

  argumentsParase(thiz, str, len, &info);
  return thiz;
}

NQArguments* NQArguments_parseUTF16(const uint16_t* str)
{
  int argcSave;
  size_t sizeSave = parseCommandLineUTF16(str, NULL, NULL, &argcSave);
  size_t argvSizeInBytes = (argcSave + 1) * sizeof(char*);
  size_t sizeInBytes = argvSizeInBytes + sizeSave;

  size_t classSize = NQ_ALIGN_UP(sizeof(NQArguments), sizeof(char*));
  NQArguments* thiz = (NQArguments*)NQMalloc(classSize + sizeInBytes);
  if (thiz == NULL)
    return NULL;

  if (argcSave == 0) {
    thiz->argc = 0;
    thiz->argv = s_argvEmpty;
  }
  else {
    thiz->argv = (char**)((char*)thiz + classSize);
    char* buffer = (char*)thiz->argv + argvSizeInBytes;
    size_t size = parseCommandLineUTF16(str, buffer, (char**)thiz->argv, &thiz->argc);
    NQ_ASSERT(thiz->argc == argcSave);
    NQ_ASSERT_UNUSED(size, size == sizeSave);
    thiz->argv[thiz->argc] = NULL;
  }

  return thiz;
}

void NQArguments_destroy(NQArguments* thiz)
{
  if (thiz != &s_argumentsEmpty)
    NQFree(thiz);
}

size_t NQArguments_count(NQArguments* thiz)
{
  return (size_t)thiz->argc;
}

const char** NQArguments_items(NQArguments* thiz)
{
  return (const char**)thiz->argv;
}

const char* NQArguments_at(NQArguments* thiz, size_t index)
{
  if (index <= NQ_INT_MAX && (int)index < thiz->argc)
    return thiz->argv[index];
  return NULL;
}

bool NQArguments_has(NQArguments* thiz, const char* value)
{
  for (int i = 0; i < thiz->argc; i++) {
    if (!strcmp(thiz->argv[i], value))
      return true;
  }
  return false;
}

bool NQArguments_hasLKey(NQArguments* thiz, const char* value)
{
  for (int i = 0; i < thiz->argc; i++) {
    const char* p1 = thiz->argv[i];

    if (*p1 != '-')
      continue;
    p1++;

    if (*p1 != '-')
      continue;
    p1++;

    if (*p1 == '=' || *p1 == '\0')
      continue;

    const char* p2 = value;
    if (*p1 != *p2)
      continue;

    for(;;) {
      p1++;
      p2++;

      if (*p1 != *p2) {
        if (*p1 == '=' && *p2 == '\0')
          return true;
        break;
      }

      if (*p1 == '\0')
        return true;
    }
  }

  return false;
}

void NQMainArgumentsInit(int argc, const char* argv[])
{
  NQArguments* arguments = NQArguments_create(argc, argv);
  if (arguments != NULL)
    s_arguments = arguments;
}

size_t NQMainArgumentsCount(void)
{
  return NQArguments_count(s_arguments);
}

const char* NQMainArgumentsAt(size_t index)
{
  return NQArguments_at(s_arguments, index);
}

#ifdef NQ_OS_WINDOWS

void NQMainArgumentsInitWin(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT iCmdShow)
{
  s_hInstance = hInstance;
  s_hPrevInstance = hPrevInstance;
  s_lpCmdLine = lpCmdLine;
  s_lpCmdLineW = NULL;
  s_iCmdShow = iCmdShow;

  PWSTR str = GetCommandLineW();
  NQArguments* arguments = NQArguments_parseUTF16((uint16_t*)str);
  s_arguments = (arguments != NULL) ? arguments : &s_argumentsEmpty;
}

void NQMainArgumentsInitWinW(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, INT iCmdShow)
{
  s_hInstance = hInstance;
  s_hPrevInstance = hPrevInstance;
  s_lpCmdLine = NULL;
  s_lpCmdLineW = lpCmdLine;
  s_iCmdShow = iCmdShow;

  PWSTR str = GetCommandLineW();
  NQArguments* arguments = NQArguments_parseUTF16((uint16_t*)str);
  s_arguments = (arguments != NULL) ? arguments : &s_argumentsEmpty;
}

HINSTANCE NQGetInstance(void)
{
  if (s_hInstance == NULL)
    s_hInstance = GetModuleHandleW(NULL);
  return s_hInstance;
}

HINSTANCE NQGetPrevInstance(void)
{
  return s_hPrevInstance;
}

INT NQGetCmdShow(void)
{
  return s_iCmdShow;
}

#endif

void NQMainArgumentsFinalize(void)
{
  if (s_arguments != &s_argumentsEmpty) {
    NQFree(s_arguments);
    s_arguments = &s_argumentsEmpty;
  }
}

NQArguments* NQArgumentsGetMain(void)
{
  return s_arguments;
}
