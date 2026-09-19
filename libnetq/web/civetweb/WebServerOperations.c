/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#define NQ_LOG_TAG "WebServerImpl"

#include "config.h"

#ifdef WITH_CIVETWEB

#include <libnetq/web/civetweb/CivetWebAdapter.h>

#include <libnetq/UrlQuery.h>
#include <libnetq/KeyVal.h>
#include <libnetq/Malloc.h>
#include <libnetq/Log.h>
#include <libnetq/http/HttpHeader.h>
#include <libnetq/http/HttpStatus.h>
#include <libnetq/string/Sprintf.h>
#include <libnetq/Assert.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/Mutex.h>
#include <libnetq/BufferBuilder.h>
#include <libnetq/web/WebRequest.h>
#include <libnetq/web/WebResponse.h>
#include <libnetq/web/WebServer.h>
#include <libnetq/web/WebSocket.h>

struct CivetWeRequest {
  struct mg_connection* conn;
  NQWebRequest base;
  NQUrlQuery* urlQuery;
};

struct CivetWeResponse {
  NQWebResponse base;
  NQKeyVal* headers;
  NQStringPrint buffer;
};

struct CivetWebSocket {
  NQWebSocket base;
  struct mg_connection* conn;
};

struct WebContextCivetWeb {
  struct CivetWeRequest request;
  struct CivetWeResponse response;
  NQWebSocket* socket;
};

static int flagsToOpcode(unsigned flags)
{
  switch (flags) {
  case WEB_WSOPCODE_CONTINUATION:
    return MG_WEBSOCKET_OPCODE_CONTINUATION;
  case WEB_WSOPCODE_TEXT:
    return MG_WEBSOCKET_OPCODE_TEXT;
  case WEB_WSOPCODE_BINARY:
    return MG_WEBSOCKET_OPCODE_BINARY;
  case WEB_WSOPCODE_CONNECTION_CLOSE:
    return MG_WEBSOCKET_OPCODE_CONNECTION_CLOSE;
  case WEB_WSOPCODE_PING:
    return MG_WEBSOCKET_OPCODE_PING;
  case WEB_WSOPCODE_PONG:
    return MG_WEBSOCKET_OPCODE_PONG;
  default:
    return (int)flags;
  }
}

static unsigned opcodeToFlags(int opcode)
{
  switch (opcode) {
  case MG_WEBSOCKET_OPCODE_CONTINUATION:
    return WEB_WSOPCODE_CONTINUATION;
  case MG_WEBSOCKET_OPCODE_TEXT:
    return WEB_WSOPCODE_TEXT;
  case MG_WEBSOCKET_OPCODE_BINARY:
    return WEB_WSOPCODE_BINARY;
  case MG_WEBSOCKET_OPCODE_CONNECTION_CLOSE:
    return WEB_WSOPCODE_CONNECTION_CLOSE;
  case MG_WEBSOCKET_OPCODE_PING:
    return WEB_WSOPCODE_PING;
  case MG_WEBSOCKET_OPCODE_PONG:
    return WEB_WSOPCODE_PONG;
  default:
    return (unsigned)opcode;
  }
}

static const char* requestGetQuery(const NQWebRequest* req, const char* name)
{
  struct CivetWeRequest* request = NQ_CONTAINER_OF(req, struct CivetWeRequest, base);
  return NQUrlQuery_value(request->urlQuery, name);
}

static const char* requestGetCookie(const NQWebRequest* req, const char* name)
{
  struct CivetWeRequest* request = NQ_CONTAINER_OF(req, struct CivetWeRequest, base);
  return NULL;
}

static const char* requestGetHeader(const NQWebRequest* req, const char* header)
{
  struct CivetWeRequest* request = NQ_CONTAINER_OF(req, struct CivetWeRequest, base);
  return mg_get_header(request->conn, header);
}

static const NQWebRequestClass kCivetWebWebRequestClass = {
  .getQuery = requestGetQuery,
  .getCookie = requestGetCookie,
  .getHeader = requestGetHeader,
};

static int requestInit(struct CivetWeRequest* request, NQWebServer* server, struct mg_connection* conn)
{
  const struct mg_request_info* rinfo = mg_get_request_info(conn);

  NQWebRequest_init(&request->base);
  request->base.clazz = &kCivetWebWebRequestClass;
  request->base.url = rinfo->local_uri;
  request->base.method = rinfo->request_method;
  request->base.version = rinfo->http_version;
  request->base.server = server;
  request->base.userdata = NULL;
  request->conn = conn;
  const char* query = strchr(rinfo->local_uri, '?');
  request->urlQuery = NQUrlQuery_create(query ? query : "?");

  return 0;
}

static void requestRelease(struct CivetWeRequest* request)
{
  if (request->urlQuery) {
    NQUrlQuery_destroy(request->urlQuery);
  }

  NQWebRequest_finalize(&request->base);
}

static bool responseSetHeader(NQWebResponse* res, const char* header, const char* value)
{
  struct CivetWeResponse* response = NQ_CONTAINER_OF(res, struct CivetWeResponse, base);
  NQKeyVal_set(response->headers, header, value);
  return true;
}

static int responseWrite(NQWebResponse* res, const void* data, size_t size)
{
  struct CivetWeResponse* response = NQ_CONTAINER_OF(res, struct CivetWeResponse, base);
  return NQStringPrint_write(&response->buffer, (const char*)data, size);
}

static int responseFlush(NQWebResponse* res)
{
  struct CivetWeResponse* response = NQ_CONTAINER_OF(res, struct CivetWeResponse, base);
  return 0;
}

static const struct NQWebResponseOperations kResponseOperations = {
  .setHeader = responseSetHeader,
  .write = responseWrite,
  .flush = responseFlush,
};

static void responseInit(struct CivetWeResponse* response, struct CivetWeRequest* request)
{
  NQWebResponse_init(&response->base, &kResponseOperations, &request->base);

  response->headers = NQKeyVal_create();
  NQStringPrint_init(&response->buffer);
}

static void responseRelease(struct CivetWeResponse* response)
{
  NQKeyVal_destroy(response->headers);
  NQStringPrint_finalize(&response->buffer);

  NQWebResponse_finalize(&response->base);
}

static bool contextInit(struct WebContextCivetWeb* thiz, NQWebServer* server, struct mg_connection* conn)
{
  requestInit(&thiz->request, server, conn);
  responseInit(&thiz->response, &thiz->request);
  thiz->socket = NULL;
  return true;
}

static void contextFinalize(struct WebContextCivetWeb* thiz)
{
  responseRelease(&thiz->response);
  requestRelease(&thiz->request);
}

static void websocketUpgrade(NQWebSocket* sock, const struct mg_connection* conn)
{
  struct CivetWebSocket* impl = NQ_CONTAINER_OF(sock, struct CivetWebSocket, base);
  impl->conn = (struct mg_connection*)conn;
}

static int websocketSend(NQWebSocket* sock, const uint8_t* data, size_t size, unsigned flags)
{
  struct CivetWebSocket* thiz = (struct CivetWebSocket*)sock;
  return mg_websocket_write(thiz->conn, flagsToOpcode(flags & 0x0f), (const char*)data, size);
}

static void websocketClose(NQWebSocket* sock, uint16_t statusCode)
{
  struct CivetWebSocket* thiz = (struct CivetWebSocket*)sock;
  uint8_t data[2];
  NQPutUint16BE(data, statusCode);
  mg_websocket_write(thiz->conn, MG_WEBSOCKET_OPCODE_CONNECTION_CLOSE, data, sizeof(data));
}

static void websocketRelease(NQWebSocket* sock)
{
  struct CivetWebSocket* thiz = (struct CivetWebSocket*)sock;
  NQFree(thiz);
}

static const struct NQWebSocketClass kCivetWebWebSocketClass = {
  .name = "CivetWeb",
  .send = websocketSend,
  .close = websocketClose,
  .release = websocketRelease,
};

static struct CivetWebSocket* websocketCreate(NQWebServer* server)
{
  struct CivetWebSocket* thiz = (struct CivetWebSocket*)NQMalloc(sizeof(struct CivetWebSocket));
  if (thiz == NULL)
	return NULL;

  thiz->base.clazz = &kCivetWebWebSocketClass;
  thiz->base.looper = NQWebServer_looper(server);
  thiz->base.userdata = NULL;

  thiz->conn = NULL;

  return thiz;
}

static int websocketConnect(const struct mg_connection* conn, void* userdata)
{
  NQWebServer* server = (NQWebServer*)userdata;

  const char* host = mg_get_header(conn, NQHTTP_HEADER_HOST);
  if (host == NULL) {
    return 1;
  }

  struct WebContextCivetWeb* context = (struct WebContextCivetWeb*)NQMalloc(sizeof(*context));
  if (!contextInit(context, server, (struct mg_connection*)conn)) {
    return 1;
  }

  struct CivetWebSocket* ws = websocketCreate(server);
  if (ws == NULL)
    return 1;

  context->socket = &ws->base;
  if (!NQWebServer_initSocket(server, &context->request.base, &ws->base)) {
    NQWebSocket_release(&ws->base);
    return 1;
  }

  websocketUpgrade(context->socket, conn);
  mg_set_user_connection_data((struct mg_connection*)conn, context);
  return 0;
}

static void websocketOpen(struct mg_connection* conn, void* userdata)
{
  NQWebServer* server = (NQWebServer*)userdata;
  struct WebContextCivetWeb* ctx = (struct WebContextCivetWeb*)mg_get_user_connection_data(conn);

  NQ_ASSERT(ctx->request.conn == conn);
  NQ_ASSERT(ctx->request.base.server == server);

  NQWebSocket_doOpen(ctx->socket);
}

static int websocketRecive(struct mg_connection* conn, int opcode, char* data, size_t datasize, void* userdata)
{
  NQWebServer* server = (NQWebServer*)userdata;
  struct WebContextCivetWeb* ctx = (struct WebContextCivetWeb*)mg_get_user_connection_data(conn);
  NQ_ASSERT(ctx->request.conn == conn);
  NQ_ASSERT(ctx->request.base.server == server);
  NQWebSocket_doReceive(ctx->socket, (uint8_t*)data, datasize, opcodeToFlags(opcode & 0x0f));
  return 1;
}

static void websocketCloseHandler(const struct mg_connection* conn, void* userdata)
{
  NQWebServer* server = (NQWebServer*)userdata;
  struct WebContextCivetWeb* ctx = (struct WebContextCivetWeb*)mg_get_user_connection_data(conn);

  NQ_ASSERT(ctx->request.conn == conn);
  NQ_ASSERT(ctx->request.base.server == server);

  NQWebSocket_doClose(ctx->socket, 0);
  NQWebSocket_release(ctx->socket);
  ctx->socket = NULL;
  contextFinalize(ctx);
  NQFree(ctx);
}

static int authorizationHandler(struct mg_connection* conn, void* userdata)
{
  return 1; // Authorization successful - request continues
  // return 0; // Authorization failed - sends 401/403
  // return -1; // Not handled - uses default auth logic
}

static int requestHandler(struct mg_connection* conn, void* userdata)
{
  NQWebServer* server = (NQWebServer*)userdata;

  const char* host = mg_get_header(conn, NQHTTP_HEADER_HOST);
  if (host == NULL) {
    mg_send_http_error(conn, NQ_HTTP_NOT_FOUND, "Not found");
    return NQ_HTTP_NOT_FOUND;
  }

  struct WebContextCivetWeb context;
  if (!contextInit(&context, server, conn)) {
    mg_send_http_error(conn, NQ_HTTP_INTERNAL_SERVER_ERROR, "Server error");
    return NQ_HTTP_INTERNAL_SERVER_ERROR;
  }

  if (!NQWebServer_initRequest(server, &context.request.base)) {
    contextFinalize(&context);
    mg_send_http_error(conn, NQ_HTTP_NOT_FOUND, "Not found");
    return NQ_HTTP_NOT_FOUND;
  }

  NQWebRequest* request = &context.request.base;
  NQWebResponse* response = &context.response.base;

  NQHttpStatistics_inc(server->statistics, NQWebRequest_method(request), NQWebRequest_url(request));
  char receiveBuffer[2048];
  while (true) {
    int ret = mg_read(conn, receiveBuffer, sizeof(receiveBuffer));
    if (ret == 0)
      break;
    if (ret < 0) {
      mg_send_http_error(conn, NQ_HTTP_INTERNAL_SERVER_ERROR, "Server error");
      return NQ_HTTP_INTERNAL_SERVER_ERROR;
    }
    if (request->operations->receive) {
      size_t n = request->operations->receive(request, receiveBuffer, ret);
    }
  }

  int statusCode = NQ_HTTP_OK;
  if (request->operations && request->operations->handler) {
    statusCode = request->operations->handler(request, response);
    NQWebResponse_flush(response);
  }

  mg_response_header_start(conn, statusCode);
  NQKeyValIter* iter = NQKeyVal_begin(context.response.headers);
  while (iter != NULL) {
    const char* key = NQKeyValIter_key(iter);
    const char* val = NQKeyValIter_val(iter);
    mg_response_header_add(conn, key, val, -1);
    iter = NQKeyValIter_next(iter);
  }
  mg_response_header_send(conn);

  char* data = NQStringPrint_characters(&context.response.buffer);
  size_t size = NQStringPrint_length(&context.response.buffer);
  mg_write(conn, data, size);

  contextFinalize(&context);
  return statusCode;
}

static int logMessage(const struct mg_connection* conn, const char* message)
{
  NQ_LOGI("%s", message);
  return 1;
}

static int logAccess(const struct mg_connection* conn, const char* message)
{
  NQ_LOGI("%s", message);
  return 1;
}

static NQ_MUTEX_DEFINE(g_mutex);
static unsigned g_initCounter = 0;

static bool serverInit(NQWebServer* thiz)
{
  NQMutex_lock(&g_mutex);
  if (g_initCounter++ == 0)
    mg_init_library(0);
  thiz->userdata = NULL;
  NQMutex_unlock(&g_mutex);
  return true;
}

static void serverRelease(NQWebServer* thiz)
{
  NQMutex_lock(&g_mutex);
  if (--g_initCounter == 0)
    mg_exit_library();
  NQMutex_unlock(&g_mutex);
}

static int serverStart(NQWebServer* thiz)
{
  uint16_t port = NQUrlHost_port(thiz->host);
  char portBuf[6];
  NQSnprintf(portBuf, sizeof(portBuf), "%i", port);

  const char* options[] = {
    "listening_ports",    portBuf,
    "num_threads",        "10",
    "request_timeout_ms", "120000",
    NULL,                 NULL,
  };

  struct mg_callbacks callbacks;
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.log_message = logMessage;
  callbacks.log_access = logAccess;

  struct mg_init_data initData = { 0 };
  initData.callbacks = &callbacks;
  initData.user_data = thiz;
  initData.configuration_options = options;

  struct mg_error_data errorData = { 0 };
  char errBuf[256] = { 0 };
  errorData.text = errBuf;
  errorData.text_buffer_size = sizeof(errBuf);

  struct mg_context* ctx = mg_start2(&initData, &errorData);
  if (!ctx) {
    NQ_LOGE("Cannot start server: %s", errBuf);
    return -NQ_ENOMEM;
  }

  mg_set_auth_handler(ctx, "/", authorizationHandler, thiz);
  mg_set_request_handler(ctx, "/", requestHandler, thiz);
  mg_set_websocket_handler(ctx, "/", websocketConnect, websocketOpen, websocketRecive, websocketCloseHandler, thiz);

  thiz->userdata = ctx;
  return 0;
}

static int serverStop(NQWebServer* thiz)
{
  struct mg_context* ctx = (struct mg_context*)thiz->userdata;
  mg_stop(ctx);
  return 0;
}

extern NQWebServerOperations kCivetWebServerOperations;
NQWebServerOperations kCivetWebServerOperations = {
  .name = "CivetWeb",
  .init = serverInit,
  .start = serverStart,
  .stop = serverStop,
  .release = serverRelease,
};

#endif /* WITH_CIVETWEB */
