/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_WEB_WEBCOOKIE_H
#define _LIBNETQ_WEB_WEBCOOKIE_H

#include <libnetq/Basic.h>

#define NQ_WEBCOOKIE_PREFIX_SECURE "__Secure-"
#define NQ_WEBCOOKIE_PREFIX_HOST   "__Host-"

typedef enum NQWebCookieSameSite NQWebCookieSameSite;
enum NQWebCookieSameSite {
  NQ_WEBCOOKIE_SAMESITE_UNSET = 0,  /* attribute omitted (browser default: Lax) */
  NQ_WEBCOOKIE_SAMESITE_STRICT,
  NQ_WEBCOOKIE_SAMESITE_LAX,
  NQ_WEBCOOKIE_SAMESITE_NONE        /* forces Secure */
};

typedef struct NQWebCookie NQWebCookie;
struct NQWebCookie {
  const char* name;      /* required, RFC 6265 token */
  const char* value;     /* sent verbatim, NULL = empty; no CTL, space, '"', ',', ';', '\\' */
  const char* path;      /* NULL = omit; no CTL or ';' */
  const char* domain;    /* NULL = omit; no CTL or ';' */

  int64_t maxAge;        /* seconds; 0 = omit, <0 = "Max-Age=0" (delete now) */
  int64_t expires;       /* unix time in seconds; 0 = omit */

  bool httpOnly;
  bool secure;
  bool partitioned;      /* forces Secure */
  NQWebCookieSameSite sameSite;
};

#endif /* _LIBNETQ_WEB_WEBCOOKIE_H */
