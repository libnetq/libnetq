/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/web/WebResponse.h"

#include <libnetq/string/String.h>
#include <libnetq/http/HttpHeader.h>
#include <libnetq/Malloc.h>
#include <libnetq/Time.h>
#include <libnetq/web/WebRequest.h>
#include <libnetq/web/WebServer.h>

static int platformWrite(NQWebWriter* writer, const void* data, size_t size)
{
  NQWebResponse* thiz = NQ_CONTAINER_OF(writer, struct NQWebResponse, lastWriter);
  return thiz->operations->write(thiz, data, size);
}

static int platformFlush(NQWebWriter* writer)
{
  NQWebResponse* thiz = NQ_CONTAINER_OF(writer, struct NQWebResponse, lastWriter);
  return thiz->operations->flush(thiz);
}

static const NQWebWriterOperations kPlatformWriterOperations =
{
  .init = NULL,
  .write = platformWrite,
  .flush = platformFlush,
  .release = NULL,
};

void NQWebResponse_init(NQWebResponse* thiz, const struct NQWebResponseOperations* operations, NQWebRequest* request)
{
  thiz->operations = operations;
  thiz->server = NQWebRequest_server(request);
  thiz->request = request;

  NQStringPrint_init(&thiz->printfBuffer);

  thiz->firstWriter = &thiz->lastWriter;
  thiz->lastWriter.operations = &kPlatformWriterOperations;
  thiz->lastWriter.next = NULL;
  thiz->lastWriter.request = request;
  thiz->lastWriter.userdata = NULL;
}

void NQWebResponse_finalize(NQWebResponse* thiz)
{
  NQWebWriter* iter = thiz->firstWriter;
  while (iter) {
    NQWebWriter* next = iter->next;
    if (iter->operations->release)
      iter->operations->release(iter);
    if (iter != &thiz->lastWriter)
      NQFree(iter);
    iter = next;
  }

  NQStringPrint_finalize(&thiz->printfBuffer);
}

int NQWebResponse_printf(NQWebResponse* thiz, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int ret = NQWebResponse_vprintf(thiz, fmt, args);
  va_end(args);
  return ret;
}

int NQWebResponse_vprintf(NQWebResponse* thiz, const char* fmt, va_list args)
{
  int result = NQStringPrint_vprintf(&thiz->printfBuffer, fmt, args);
  if (result > 0) {
    const char* characters = NQStringPrint_characters(&thiz->printfBuffer);
    size_t length = NQStringPrint_length(&thiz->printfBuffer);
    result = NQWebWriter_write(thiz->firstWriter, characters, length);
    NQStringPrint_reset(&thiz->printfBuffer);
  }
  return result;
}

int NQWebResponse_write(NQWebResponse* thiz, const void* data, size_t size)
{
  return NQWebWriter_write(thiz->firstWriter, data, size);
}

int NQWebResponse_flush(NQWebResponse* thiz)
{
  return NQWebWriter_flush(thiz->firstWriter);
}

/* RFC 7230 tchar */
static bool isTokenChar(unsigned char c)
{
  if (c <= 0x20 || c >= 0x7f)
    return false;
  return NQStrchr("\"(),/:;<=>?@[\\]{}", c) == NULL;
}

static bool isToken(const char* s)
{
  if (s == NULL || *s == '\0')
    return false;
  for (; *s; s++) {
    if (!isTokenChar((unsigned char)*s))
      return false;
  }
  return true;
}

/* RFC 6265 cookie-octet */
static bool isCookieValue(const char* s)
{
  for (; *s; s++) {
    unsigned char c = (unsigned char)*s;
    if (c <= 0x20 || c >= 0x7f || c == '"' || c == ',' || c == ';' || c == '\\')
      return false;
  }
  return true;
}

/* RFC 6265 av-octet: any CHAR except CTLs or ';' */
static bool isAttributeValue(const char* s)
{
  for (; *s; s++) {
    unsigned char c = (unsigned char)*s;
    if (c < 0x20 || c >= 0x7f || c == ';')
      return false;
  }
  return true;
}

bool NQWebResponse_addHeader(NQWebResponse* thiz, const char* header, const char* value)
{
  if (!NQStrcmp(NQHTTP_HEADER_CONTENT_TYPE, header)) {
    NQWebWriter* writerChain = NQWebServer_createWriterChain(thiz->server, value, thiz->request);
    if (writerChain) {
      NQWebWriter* lastWriter = writerChain;
      while (lastWriter->next)
        lastWriter = lastWriter->next;
      lastWriter->next = thiz->firstWriter;
      thiz->firstWriter = writerChain;
    }
  }

  return thiz->operations->addHeader(thiz, header, value);
}

static const char* sameSiteName(NQWebCookieSameSite sameSite)
{
  switch (sameSite) {
  case NQ_WEBCOOKIE_SAMESITE_STRICT:
    return "Strict";
  case NQ_WEBCOOKIE_SAMESITE_LAX:
    return "Lax";
  case NQ_WEBCOOKIE_SAMESITE_NONE:
    return "None";
  default:
    return NULL;
  }
}

static int printExpires(NQStringPrint* out, int64_t expires)
{
  static const char* const kDays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
  static const char* const kMonths[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  time_t t = (time_t)expires;
  NQDataTime tm;
  NQGmtime(&t, &tm);
  if (tm.tm_wday < 0 || tm.tm_wday > 6 || tm.tm_mon < 0 || tm.tm_mon > 11)
    return -1;

  return NQStringPrint_printf(out, "; Expires=%s, %02d %s %04ld %02d:%02d:%02d GMT",
                              kDays[tm.tm_wday],
                              tm.tm_mday,
                              kMonths[tm.tm_mon],
                              (long)tm.tm_year + NQ_TM_YEAR_BASE,
                              tm.tm_hour,
                              tm.tm_min,
                              tm.tm_sec);
}

bool NQWebResponse_addCookie(NQWebResponse* thiz, const NQWebCookie* cookie)
{
  const char* value = cookie->value ? cookie->value : "";
  if (!isToken(cookie->name) || !isCookieValue(value))
    return false;
  if (cookie->path && !isAttributeValue(cookie->path))
    return false;
  if (cookie->domain && !isAttributeValue(cookie->domain))
    return false;

  const char* sameSite = sameSiteName(cookie->sameSite);
  bool secure = cookie->secure || cookie->partitioned || cookie->sameSite == NQ_WEBCOOKIE_SAMESITE_NONE;

  /* Browsers drop prefixed cookies that break these rules, silently. */
  if (NQCStrStartsWith(cookie->name, NQ_WEBCOOKIE_PREFIX_SECURE) && !secure)
    return false;
  if (NQCStrStartsWith(cookie->name, NQ_WEBCOOKIE_PREFIX_HOST) && (!secure || cookie->domain || cookie->path == NULL || NQStrcmp(cookie->path, "/")))
    return false;

  NQStringPrint out;
  NQStringPrint_init(&out);

  bool ok = NQStringPrint_printf(&out, "%s=%s", cookie->name, value) >= 0;
  if (ok && cookie->path)
    ok = NQStringPrint_printf(&out, "; Path=%s", cookie->path) >= 0;
  if (ok && cookie->domain)
    ok = NQStringPrint_printf(&out, "; Domain=%s", cookie->domain) >= 0;
  if (ok && cookie->maxAge != 0)
    ok = NQStringPrint_printf(&out, "; Max-Age=%lld", (long long)(cookie->maxAge < 0 ? 0 : cookie->maxAge)) >= 0;
  if (ok && cookie->expires != 0)
    ok = printExpires(&out, cookie->expires) >= 0;
  if (ok && secure)
    ok = NQStringPrint_printf(&out, "; Secure") >= 0;
  if (ok && cookie->httpOnly)
    ok = NQStringPrint_printf(&out, "; HttpOnly") >= 0;
  if (ok && sameSite)
    ok = NQStringPrint_printf(&out, "; SameSite=%s", sameSite) >= 0;
  if (ok && cookie->partitioned)
    ok = NQStringPrint_printf(&out, "; Partitioned") >= 0;

  if (ok)
    ok = NQWebResponse_addHeader(thiz, NQHTTP_HEADER_SET_COOKIE, NQStringPrint_characters(&out));

  NQStringPrint_finalize(&out);
  return ok;
}

bool NQWebResponse_deleteCookie(NQWebResponse* thiz, const char* name, const char* path, const char* domain)
{
  NQWebCookie cookie = { 0 };
  cookie.name = name;
  cookie.path = path;
  cookie.domain = domain;
  cookie.maxAge = -1;
  cookie.expires = 1;  /* Thu, 01 Jan 1970 00:00:01 GMT, for clients without Max-Age */
  cookie.secure = name && (NQCStrStartsWith(name, NQ_WEBCOOKIE_PREFIX_SECURE) || NQCStrStartsWith(name, NQ_WEBCOOKIE_PREFIX_HOST));
  return NQWebResponse_addCookie(thiz, &cookie);
}
