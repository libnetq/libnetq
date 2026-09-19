/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_WEB_CIVETWEB_CIVETWEBADAPTER_H
#define _LIBNETQ_WEB_CIVETWEB_CIVETWEBADAPTER_H

#include <libnetq/Basic.h>

#include <civetweb.h>

#ifdef CIVETWEB_VERSION_MAJOR
#define NQ_CIVETWEB_VERSION NQ_VERSION_DEC(CIVETWEB_VERSION_MAJOR, CIVETWEB_VERSION_MINOR, CIVETWEB_VERSION_PATCH)
#else
#define NQ_CIVETWEB_VERSION NQ_VERSION_DEC(1, 9, 0)
#endif

#if NQ_CIVETWEB_VERSION < NQ_VERSION_DEC(1, 12, 0)
struct mg_init_data {
  const struct mg_callbacks* callbacks;
  void* user_data;
  const char** configuration_options;
};
struct mg_error_data {
  unsigned code;
  unsigned code_sub;
  char* text;
  size_t text_buffer_size;
};
#endif

#if NQ_CIVETWEB_VERSION < NQ_VERSION_DEC(1, 15, 0)
static inline struct mg_context* mg_start2(struct mg_init_data* init, struct mg_error_data* error)
{
  if (error != NULL) {
    error->code = 0;
    error->text = NULL;
    error->text_buffer_size = 0;
  }
  return mg_start(init->callbacks, init->user_data, init->configuration_options);
}
#endif

#if NQ_CIVETWEB_VERSION < NQ_VERSION_DEC(1, 13, 0)
static inline int mg_response_header_start(struct mg_connection* conn, int status)
{
  NQ_UNUSED_PARAM(conn);
  NQ_UNUSED_PARAM(status);
  return -1;
}
static inline int mg_response_header_add(struct mg_connection* conn, const char* header, const char* value, int vlen)
{
  NQ_UNUSED_PARAM(conn);
  NQ_UNUSED_PARAM(header);
  NQ_UNUSED_PARAM(value);
  NQ_UNUSED_PARAM(vlen);
  return -1;
}
static inline int mg_response_header_send(struct mg_connection* conn)
{
  NQ_UNUSED_PARAM(conn);
  return -1;
}
#endif

#if NQ_CIVETWEB_VERSION < NQ_VERSION_DEC(1, 10, 0)
#define MG_WEBSOCKET_OPCODE_CONTINUATION     WEBSOCKET_OPCODE_CONTINUATION
#define MG_WEBSOCKET_OPCODE_TEXT             WEBSOCKET_OPCODE_TEXT
#define MG_WEBSOCKET_OPCODE_BINARY           WEBSOCKET_OPCODE_BINARY
#define MG_WEBSOCKET_OPCODE_CONNECTION_CLOSE WEBSOCKET_OPCODE_CONNECTION_CLOSE
#define MG_WEBSOCKET_OPCODE_PING             WEBSOCKET_OPCODE_PING
#define MG_WEBSOCKET_OPCODE_PONG             WEBSOCKET_OPCODE_PONG
static inline int mg_send_http_error(struct mg_connection* conn, int statusCode, const char* fmt, ...)
{
  return -1;
}
#endif

#if NQ_CIVETWEB_VERSION < NQ_VERSION_DEC(1, 9, 0)
static inline unsigned mg_init_library(unsigned features)
{
  return 0;
}
static inline unsigned mg_exit_library(void)
{
  return 0;
}
#endif

#endif /* _LIBNETQ_WEB_CIVETWEB_CIVETWEBADAPTER_H */
