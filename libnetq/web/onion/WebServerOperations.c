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

#ifdef WITH_ONION

#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>

#include <onion/onion.h>
#include <onion/dict.h>
#include <onion/block.h>
#include <onion/handler.h>
#include <onion/http.h>
#include <onion/request.h>
#include <onion/response.h>
/* For onion_request::connection.fd and onion_response::flags: websockets are
 * framed here (onion's own websocket code needs GnuTLS for the handshake). */
#include <onion/types_internal.h>

#include <libnetq/BufferBuilder.h>
#include <libnetq/List.h>
#include <libnetq/Malloc.h>
#include <libnetq/Log.h>
#include <libnetq/Mutex.h>
#include <libnetq/Thread.h>
#include <libnetq/URL.h>
#include <libnetq/http/HttpHeader.h>
#include <libnetq/http/HttpStatus.h>
#include <libnetq/string/Sprintf.h>
#include <libnetq/string/String.h>
#include <libnetq/string/NameValue.h>
#include <libnetq/Assert.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/web/WebRequest.h>
#include <libnetq/web/WebResponse.h>
#include <libnetq/web/WebServer.h>
#include <libnetq/web/WebSocket.h>
#include <libnetq/web/WebSocketCalcAccept.h>
#include <libnetq/web/WebSocketFrame.h>

/* Pool threads; every open websocket keeps one of them busy until it closes. */
#define ONION_MAX_THREADS        32
#define ONION_TIMEOUT_MS         120000
#define ONION_START_TIMEOUT_MS   2000
#define ONION_WS_VERSION         "13"
#define ONION_WS_MAX_FRAME_SIZE  (16 * 1024 * 1024)

struct OnionWebServer {
  onion* server;
  NQMutex mutex;
  NQListHead sockets;
};

struct OnionWebRequest {
  NQWebRequest base;
  onion_request* req;
};

struct OnionWebResponse {
  NQWebResponse base;
  NQNameValue headers;
  NQStringPrint buffer;
};

struct OnionWebSocket {
  NQWebSocket base;
  int fd;
  NQMutex mutex;
};

struct OnionWebContext {
  struct OnionWebRequest request;
  struct OnionWebResponse response;
  struct OnionWebSocket* socket;
  struct OnionWebServer* owner;
  NQListHead list;
};

static const char* requestGetQuery(const NQWebRequest* req, const char* name)
{
  struct OnionWebRequest* request = NQ_CONTAINER_OF(req, struct OnionWebRequest, base);
  return onion_request_get_query(request->req, name);
}

static const char* requestGetCookie(const NQWebRequest* req, const char* name)
{
  struct OnionWebRequest* request = NQ_CONTAINER_OF(req, struct OnionWebRequest, base);
  return onion_request_get_cookie(request->req, name);
}

static const char* requestGetHeader(const NQWebRequest* req, const char* header)
{
  struct OnionWebRequest* request = NQ_CONTAINER_OF(req, struct OnionWebRequest, base);
  return onion_request_get_header(request->req, header);
}

static const NQWebRequestClass kOnionWebRequestClass = {
  .getQuery = requestGetQuery,
  .getCookie = requestGetCookie,
  .getHeader = requestGetHeader,
};

static void requestInit(struct OnionWebRequest* request, NQWebServer* server, onion_request* req)
{
  NQWebRequest_init(&request->base);
  request->base.clazz = &kOnionWebRequestClass;
  request->base.url = onion_request_get_fullpath(req);
  request->base.method = onion_request_get_method_name(req);
  request->base.version = (onion_request_get_flags(req) & OR_HTTP11) ? "HTTP/1.1" : "HTTP/1.0";
  request->base.server = server;
  request->base.userdata = NULL;
  request->req = req;
}

static void requestRelease(struct OnionWebRequest* request)
{
  NQWebRequest_finalize(&request->base);
}

static bool responseAddHeader(NQWebResponse* res, const char* header, const char* value)
{
  struct OnionWebResponse* response = NQ_CONTAINER_OF(res, struct OnionWebResponse, base);
  return NQNameValue_add(&response->headers, header, value);
}

static int responseWrite(NQWebResponse* res, const void* data, size_t size)
{
  struct OnionWebResponse* response = NQ_CONTAINER_OF(res, struct OnionWebResponse, base);
  return NQStringPrint_write(&response->buffer, (const char*)data, size);
}

static int responseFlush(NQWebResponse* res)
{
  NQ_UNUSED_PARAM(res);
  return 0;
}

static const struct NQWebResponseOperations kResponseOperations = {
  .addHeader = responseAddHeader,
  .write = responseWrite,
  .flush = responseFlush,
};

static void responseInit(struct OnionWebResponse* response, struct OnionWebRequest* request)
{
  NQWebResponse_init(&response->base, &kResponseOperations, &request->base);
  NQNameValue_init(&response->headers);
  NQStringPrint_init(&response->buffer);
}

static void responseRelease(struct OnionWebResponse* response)
{
  NQNameValue_finalize(&response->headers);
  NQStringPrint_finalize(&response->buffer);
  NQWebResponse_finalize(&response->base);
}

static void contextInit(struct OnionWebContext* thiz, NQWebServer* server, onion_request* req)
{
  requestInit(&thiz->request, server, req);
  responseInit(&thiz->response, &thiz->request);
  thiz->socket = NULL;
  thiz->owner = (struct OnionWebServer*)server->userdata;
  NQListHead_init(&thiz->list);
}

static void contextFinalize(struct OnionWebContext* thiz)
{
  responseRelease(&thiz->response);
  requestRelease(&thiz->request);
}

static onion_connection_status sendStatus(onion_response* res, int statusCode, const char* message)
{
  onion_response_set_code(res, statusCode);
  onion_response_set_header(res, NQHTTP_HEADER_CONTENT_TYPE, "text/plain; charset=utf-8");
  onion_response_write0(res, message);
  return OCS_PROCESSED;
}

/* Request bodies: onion parses them itself, so pass on what it keeps of the raw data. */

static void feedData(NQWebRequest* request, const char* data, size_t size)
{
  while (size != 0) {
    size_t n = request->operations->receive(request, data, size);
    if (n == 0 || n > size)
      break;
    data += n;
    size -= n;
  }
}

static void feedFile(NQWebRequest* request, const char* filename)
{
  FILE* file = fopen(filename, "rb");
  if (file == NULL) {
    NQ_LOGE("Cannot open request body '%s'", filename);
    return;
  }

  char buffer[4096];
  size_t n;
  while ((n = fread(buffer, 1, sizeof(buffer), file)) != 0)
    feedData(request, buffer, n);
  fclose(file);
}

struct FormEncoder {
  NQStringPrint print;
  bool first;
};

static void formEncodeValue(NQStringPrint* print, const char* value)
{
  size_t length = NQStrlen(value);
  int size = NQFormURLEncode(value, length, NULL, 0);
  if (size <= 0)
    return;
  char* encoded = (char*)NQMalloc((size_t)size + 1);
  if (encoded == NULL)
    return;
  NQFormURLEncode(value, length, encoded, (size_t)size + 1);
  NQStringPrint_write(print, encoded, (size_t)size);
  NQFree(encoded);
}

static void formEncodeEntry(void* data, const char* key, const void* value, int flags)
{
  struct FormEncoder* encoder = (struct FormEncoder*)data;
  if ((flags & OD_TYPE_MASK) != OD_STRING)
    return;
  if (!encoder->first)
    NQStringPrint_write(&encoder->print, "&", 1);
  encoder->first = false;
  formEncodeValue(&encoder->print, key);
  NQStringPrint_write(&encoder->print, "=", 1);
  formEncodeValue(&encoder->print, (const char*)value);
}

static void requestFeedBody(NQWebRequest* request, onion_request* req)
{
  if (request->operations == NULL || request->operations->receive == NULL)
    return;

  if ((onion_request_get_flags(req) & OR_METHODS) == OR_PUT) {
    /* PUT bodies are stored in a temporary file. */
    const char* filename = onion_request_get_file(req, "filename");
    if (filename != NULL)
      feedFile(request, filename);
    return;
  }

  const onion_block* data = onion_request_get_data(req);
  if (data != NULL) {
    feedData(request, onion_block_data(data), (size_t)onion_block_size(data));
    return;
  }

  const char* contentType = onion_request_get_header(req, NQHTTP_HEADER_CONTENT_TYPE);
  const onion_dict* post = onion_request_get_post_dict(req);
  if (post == NULL || contentType == NULL)
    return;

  if (strstr(contentType, "application/x-www-form-urlencoded") != NULL) {
    /* onion decodes urlencoded forms in place; encode the fields again (in key order). */
    struct FormEncoder encoder;
    NQStringPrint_init(&encoder.print);
    encoder.first = true;
    onion_dict_preorder(post, (void*)formEncodeEntry, &encoder);
    feedData(request, NQStringPrint_characters(&encoder.print), NQStringPrint_length(&encoder.print));
    NQStringPrint_finalize(&encoder.print);
  }
  else {
    NQ_LOGW("Multipart body of %s is not passed to the handler with the Onion backend", request->url);
  }
}

/* WebSocket */

static int websocketWrite(struct OnionWebSocket* thiz, uint8_t opcode, const uint8_t* data, size_t size)
{
  NQWebSocketBuffer buffer;
  NQWebSocketBuffer_init(&buffer);
  if (!NQWebSocketBuffer_reserve(&buffer, size != 0 ? size : 1)) {
    NQWebSocketBuffer_finalize(&buffer);
    return -NQ_ENOMEM;
  }
  if (size != 0)
    NQWebSocketBuffer_write(&buffer, data, size);
  NQWebSocketBuffer_complete(&buffer, opcode, true, NULL);

  const uint8_t* ptr = NQWebSocketBuffer_data(&buffer);
  size_t left = NQWebSocketBuffer_size(&buffer);

  int ret = (int)size;
  NQMutex_lock(&thiz->mutex);
  if (thiz->fd < 0)
    ret = -NQ_EIO;
  while (ret >= 0 && left != 0) {
    ssize_t n = send(thiz->fd, ptr, left, MSG_NOSIGNAL);
    if (n < 0 && errno == EINTR)
      continue;
    if (n <= 0) {
      ret = -NQ_EIO;
      break;
    }
    ptr += n;
    left -= (size_t)n;
  }
  NQMutex_unlock(&thiz->mutex);

  NQWebSocketBuffer_finalize(&buffer);
  return ret;
}

static int websocketSend(NQWebSocket* sock, const uint8_t* data, size_t size, unsigned flags)
{
  struct OnionWebSocket* thiz = NQ_CONTAINER_OF(sock, struct OnionWebSocket, base);
  /* Frames are always final, as with the CivetWeb backend. */
  return websocketWrite(thiz, (uint8_t)(flags & 0x0f), data, size);
}

static void websocketClose(NQWebSocket* sock, uint16_t statusCode)
{
  struct OnionWebSocket* thiz = NQ_CONTAINER_OF(sock, struct OnionWebSocket, base);
  uint8_t data[2];
  NQPutUint16BE(data, statusCode);
  websocketWrite(thiz, kNQWebSocketOpcodeConnectionClose, data, sizeof(data));
}

static void websocketRelease(NQWebSocket* sock)
{
  struct OnionWebSocket* thiz = NQ_CONTAINER_OF(sock, struct OnionWebSocket, base);
  NQMutex_finalize(&thiz->mutex);
  NQFree(thiz);
}

static const struct NQWebSocketClass kOnionWebSocketClass = {
  .name = "Onion",
  .send = websocketSend,
  .close = websocketClose,
  .release = websocketRelease,
};

static struct OnionWebSocket* websocketCreate(NQWebServer* server)
{
  struct OnionWebSocket* thiz = (struct OnionWebSocket*)NQMalloc(sizeof(struct OnionWebSocket));
  if (thiz == NULL)
    return NULL;

  thiz->base.clazz = &kOnionWebSocketClass;
  thiz->base.looper = NQWebServer_looper(server);
  thiz->base.userdata = NULL;
  thiz->fd = -1;
  NQMutex_init(&thiz->mutex);
  return thiz;
}

/* Reads frames from the connection until it closes; runs on the request thread. */
static uint32_t websocketLoop(struct OnionWebSocket* socket)
{
  size_t capacity = 4096;
  size_t size = 0;
  uint8_t* buffer = (uint8_t*)NQMalloc(capacity);
  if (buffer == NULL)
    return WEB_WSRES_NOMEMORY;

  uint32_t reason = WEB_WSRES_SUCCESS;
  bool running = true;
  while (running) {
    NQWebSocketHeader header;
    int headerSize = NQWebSocketHeaderParse(buffer, size, &header);
    size_t frameSize = headerSize > 0 ? (size_t)headerSize + (size_t)header.payloadSize : 0;

    if (headerSize > 0 && header.payloadSize > ONION_WS_MAX_FRAME_SIZE) {
      NQ_LOGE("WebSocket frame of %llu bytes is too large", (unsigned long long)header.payloadSize);
      websocketClose(&socket->base, 1009);
      reason = WEB_WSRES_UNKNOWN_FRAME;
      break;
    }

    if (headerSize <= 0 || size < frameSize) {
      size_t needed = frameSize > capacity ? frameSize : capacity;
      if (size == capacity && needed == capacity)
        needed = capacity * 2;
      if (needed != capacity) {
        uint8_t* grown = (uint8_t*)NQRealloc(buffer, needed);
        if (grown == NULL) {
          reason = WEB_WSRES_NOMEMORY;
          break;
        }
        buffer = grown;
        capacity = needed;
      }

      ssize_t n = recv(socket->fd, buffer + size, capacity - size, 0);
      if (n < 0 && errno == EINTR)
        continue;
      if (n <= 0) {
        reason = n == 0 ? WEB_WSRES_HANG_UP : WEB_WSRES_UNKNOWN;
        break;
      }
      size += (size_t)n;
      continue;
    }

    uint8_t* payload = buffer + headerSize;
    size_t payloadSize = (size_t)header.payloadSize;
    if (header.hasMask)
      NQWebSocketFramePayloadUnmask(header.mask, payload, payloadSize);

    switch (header.opcode) {
    case kNQWebSocketOpcodeContinuation:
    case kNQWebSocketOpcodeText:
    case kNQWebSocketOpcodeBinary:
      NQWebSocket_doReceive(&socket->base, payload, payloadSize, header.opcode);
      break;
    case kNQWebSocketOpcodePing:
      websocketWrite(socket, kNQWebSocketOpcodePong, payload, payloadSize);
      break;
    case kNQWebSocketOpcodePong:
      break;
    case kNQWebSocketOpcodeConnectionClose:
      /* Echo the status code back and stop. */
      websocketWrite(socket, kNQWebSocketOpcodeConnectionClose, payload, payloadSize >= 2 ? 2 : 0);
      running = false;
      break;
    default:
      reason = WEB_WSRES_UNKNOWN_FRAME;
      running = false;
      break;
    }

    size -= frameSize;
    memmove(buffer, buffer + frameSize, size);
  }

  NQFree(buffer);
  return reason;
}

static void serverTrackSocket(struct OnionWebContext* context, bool add)
{
  NQMutex_lock(&context->owner->mutex);
  if (add)
    NQListHead_addBack(&context->owner->sockets, &context->list);
  else
    NQListHead_remove(&context->list);
  NQMutex_unlock(&context->owner->mutex);
}

static onion_connection_status websocketHandler(NQWebServer* server, onion_request* req, onion_response* res)
{
  const char* key = onion_request_get_header(req, NQHTTP_HEADER_SEC_WEBSOCKET_KEY);
  const char* version = onion_request_get_header(req, NQHTTP_HEADER_SEC_WEBSOCKET_VERSION);
  if (key == NULL || version == NULL || NQStrcmp(version, ONION_WS_VERSION) != 0) {
    onion_response_set_header(res, NQHTTP_HEADER_SEC_WEBSOCKET_VERSION, ONION_WS_VERSION);
    return sendStatus(res, NQ_HTTP_UPGRADE_REQUIRED, "Upgrade required");
  }

  char accept[32];
  if (NQWebSocketCalcAccept(key, accept, sizeof(accept)) <= 0)
    return sendStatus(res, NQ_HTTP_BAD_REQUEST, "Bad websocket key");

  struct OnionWebContext context;
  contextInit(&context, server, req);

  struct OnionWebSocket* socket = websocketCreate(server);
  if (socket == NULL) {
    contextFinalize(&context);
    return sendStatus(res, NQ_HTTP_SERVICE_UNAVAILABLE, "Service unavailable");
  }

  context.socket = socket;
  if (!NQWebServer_initRequest(server, &context.request.base, &socket->base)) {
    NQWebSocket_release(&socket->base);
    contextFinalize(&context);
    return sendStatus(res, NQ_HTTP_NOT_FOUND, "Not found");
  }

  /* Handshake: onion writes "Connection: Upgrade" for upgraded responses. */
  onion_response_set_code(res, NQ_HTTP_SWITCHING_PROTOCOLS);
  res->flags |= OR_CONNECTION_UPGRADE | OR_LENGTH_SET;
  onion_response_set_header(res, NQHTTP_HEADER_UPGRADE, "websocket");
  onion_response_set_header(res, NQHTTP_HEADER_SEC_WEBSOCKET_ACCEPT, accept);
  onion_response_write_headers(res);
  onion_response_flush(res);

  socket->fd = req->connection.fd;
  serverTrackSocket(&context, true);

  NQWebSocket_doOpen(&socket->base);
  uint32_t reason = websocketLoop(socket);

  serverTrackSocket(&context, false);
  NQMutex_lock(&socket->mutex);
  socket->fd = -1;
  NQMutex_unlock(&socket->mutex);

  NQWebSocket_doClose(&socket->base, reason);
  NQWebSocket_release(&socket->base);
  contextFinalize(&context);
  return OCS_CLOSE_CONNECTION;
}

/* HTTP */

static bool isWebSocketUpgrade(onion_request* req)
{
  const char* upgrade = onion_request_get_header(req, NQHTTP_HEADER_UPGRADE);
  return upgrade != NULL && NQStrcasecmp(upgrade, "websocket") == 0;
}

static onion_connection_status requestHandler(void* privdata, onion_request* req, onion_response* res)
{
  NQWebServer* server = (NQWebServer*)privdata;

  if (onion_request_get_header(req, NQHTTP_HEADER_HOST) == NULL)
    return sendStatus(res, NQ_HTTP_NOT_FOUND, "Not found");

  if (isWebSocketUpgrade(req))
    return websocketHandler(server, req, res);

  struct OnionWebContext context;
  contextInit(&context, server, req);

  if (!NQWebServer_initRequest(server, &context.request.base, NULL)) {
    contextFinalize(&context);
    return sendStatus(res, NQ_HTTP_NOT_FOUND, "Not found");
  }

  NQWebRequest* request = &context.request.base;
  NQWebResponse* response = &context.response.base;

  NQHttpStatistics_inc(server->statistics, NQWebRequest_method(request), NQWebRequest_url(request));
  requestFeedBody(request, req);

  int statusCode = NQ_HTTP_OK;
  if (request->operations && request->operations->handler) {
    statusCode = request->operations->handler(request, response);
    NQWebResponse_flush(response);
  }

  if (request->operations && request->operations->release)
    request->operations->release(request);

  onion_response_set_code(res, statusCode);
  NQNameValueEntry* iter = NQNameValue_first(&context.response.headers);
  while (iter != NULL) {
    const char* name = NQNameValueEntryName(iter);
    const char* value = NQNameValueEntryValue(iter);
    onion_response_set_header(res, name, value);
    iter = NQNameValue_next(&context->response.headers, iter);
  }

  const char* data = NQStringPrint_characters(&context.response.buffer);
  size_t size = NQStringPrint_length(&context.response.buffer);
  onion_response_set_length(res, size);
  if (size != 0)
    onion_response_write(res, data, size);

  contextFinalize(&context);
  return OCS_PROCESSED;
}

/* Server */

static bool serverInit(NQWebServer* thiz)
{
  thiz->userdata = NULL;
  return true;
}

static void serverDestroy(struct OnionWebServer* impl)
{
  onion_free(impl->server);
  NQMutex_finalize(&impl->mutex);
  NQFree(impl);
}

static int serverStart(NQWebServer* thiz)
{
  if (thiz->userdata != NULL)
    return -NQ_EINVAL;

  if (thiz->tlsEnabled) {
    NQ_LOGE("TLS is not supported by the Onion backend");
    return -NQ_ENOTSUPP;
  }

  struct OnionWebServer* impl = (struct OnionWebServer*)NQZalloc(sizeof(*impl));
  if (impl == NULL)
    return -NQ_ENOMEM;

  NQMutex_init(&impl->mutex);
  NQListHead_init(&impl->sockets);

  /* O_NO_SIGTERM: leave SIGINT/SIGTERM to the application. */
  impl->server = onion_new(O_THREADED | O_DETACH_LISTEN | O_NO_SIGTERM);
  if (impl->server == NULL) {
    NQMutex_finalize(&impl->mutex);
    NQFree(impl);
    return -NQ_ENOMEM;
  }

  char port[8];
  NQSnprintf(port, sizeof(port), "%u", (unsigned)NQUrlHost_port(thiz->host));

  onion_listen_point* listenPoint = onion_http_new();
  if (listenPoint == NULL) {
    serverDestroy(impl);
    return -NQ_ENOMEM;
  }
  /* onion leaves listenfd at 0 until it binds and closes it on free, which
   * would close fd 0 of the process if binding fails. */
  listenPoint->listenfd = -1;
  onion_add_listen_point(impl->server, NULL, port, listenPoint);

  onion_set_timeout(impl->server, ONION_TIMEOUT_MS);
  onion_set_max_threads(impl->server, ONION_MAX_THREADS);
  onion_set_root_handler(impl->server, onion_handler_new(requestHandler, thiz, NULL));

  thiz->userdata = impl;

  int ret = onion_listen(impl->server);
  if (ret != 0) {
    NQ_LOGE("Cannot start server: %d", ret);
    thiz->userdata = NULL;
    serverDestroy(impl);
    return -NQ_EIO;
  }

  /* onion binds on its own thread; wait until it reports it is listening. */
  for (int waited = 0; !(onion_flags(impl->server) & O_LISTENING); waited += 10) {
    if (waited >= ONION_START_TIMEOUT_MS) {
      NQ_LOGE("Cannot listen on port %s", port);
      thiz->userdata = NULL;
      serverDestroy(impl);
      return -NQ_EIO;
    }
    NQThreadSleep(10);
  }

  return 0;
}

static int serverStop(NQWebServer* thiz)
{
  struct OnionWebServer* impl = (struct OnionWebServer*)thiz->userdata;
  if (impl == NULL)
    return 0;

  /* Open websockets block onion's threads; shut their sockets down so the threads return. */
  NQMutex_lock(&impl->mutex);
  NQListHead* iter = impl->sockets.next;
  while (iter != &impl->sockets) {
    struct OnionWebContext* context = NQ_CONTAINER_OF(iter, struct OnionWebContext, list);
    websocketClose(&context->socket->base, 1001); /* going away */
    shutdown(context->request.req->connection.fd, SHUT_RDWR);
    iter = iter->next;
  }
  NQMutex_unlock(&impl->mutex);

  onion_listen_stop(impl->server);
  thiz->userdata = NULL;
  serverDestroy(impl);
  return 0;
}

static void serverRelease(NQWebServer* thiz)
{
  serverStop(thiz);
}

extern NQWebServerOperations kOnionServerOperations;
NQWebServerOperations kOnionServerOperations = {
  .name = "Onion",
  .init = serverInit,
  .start = serverStart,
  .stop = serverStop,
  .release = serverRelease,
};

#endif /* WITH_ONION */
