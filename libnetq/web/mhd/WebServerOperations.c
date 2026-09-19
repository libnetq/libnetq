/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"

#ifdef WITH_MHD

#include <libnetq/PlatformPoll.h>
#include <microhttpd.h>

#include <libnetq/Malloc.h>
#include <libnetq/string/String.h>
#include <libnetq/http/HttpHeader.h>
#include <libnetq/Base64.h>
#include <libnetq/net/SocketHandle.h>
#include <libnetq/BufferBuilder.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/Assert.h>
#include <libnetq/Mutex.h>
#include <libnetq/URL.h>
#include <libnetq/web/WebSocketCalcAccept.h>
#include <libnetq/web/WebSocketFrame.h>
#include <libnetq/web/WebRequest.h>
#include <libnetq/web/WebResponse.h>
#include <libnetq/web/WebServer.h>
#include <libnetq/web/WebSocket.h>

#if MHD_VERSION < 0x00095300
# define MHD_USE_INTERNAL_POLLING_THREAD 0
# define MHD_ALLOW_UPGRADE 0
# define MHD_USE_ERROR_LOG 0
#endif

#if MHD_VERSION > 0x00095300
# define HAVE_MHD_STRICT_FOR_CLIENT
#endif

#if MHD_VERSION > 0x00097000
typedef enum MHD_Result MHDWebResult;
#else
typedef int MHDWebResult;
#endif

#define WS_VERSION "13"

struct MHDWebRequest {
  NQWebRequest base;

  char url[NQ_URL_MAX / 2];
  char method[16];
  char version[16];

  NQKeyVal* queryParams;
  NQKeyVal* headers;
  NQKeyVal* cookies;
};

struct MHDWebResponse {
  NQWebResponse base;

  NQKeyVal* headers;
  NQStringPrint buffer;
};

struct MHDWebSocket {
  NQWebSocket base;

  uint16_t statusCode;
  uint8_t data[2048];
  uint32_t size;
  NQMutex mutex;
  NQListHead outputList;

  const char* extraIn;
  size_t extraInSize;
  MHD_socket handle;
  struct MHD_UpgradeResponseHandle* urh;
};

struct MHDWebContext {
  struct MHD_Connection* connection;

  struct MHDWebRequest request;
  struct MHDWebResponse response;
  NQWebSocket* socket;
};

struct BufferEntry {
  uint8_t opcode;
  NQListHead list;
  NQWebSocketBuffer buffer;
};

struct MHDRequestParams {
  struct MHD_Connection* connection;
  const char* url;
  const char* method;
  const char* version;
};

static const char* requestGetQuery(const NQWebRequest* req, const char* name)
{
  struct MHDWebRequest* request = NQ_CONTAINER_OF(req, struct MHDWebRequest, base);
  return NQKeyVal_get(request->queryParams, name);
}

static const char* requestGetCookie(const NQWebRequest* req, const char* name)
{
  struct MHDWebRequest* request = NQ_CONTAINER_OF(req, struct MHDWebRequest, base);
  return NQKeyVal_get(request->cookies, name);
}

static const char* requestGetHeader(const NQWebRequest* req, const char* header)
{
  struct MHDWebRequest* request = NQ_CONTAINER_OF(req, struct MHDWebRequest, base);
  return NQKeyVal_get(request->headers, header);
}

static const struct NQWebRequestClass kMHDWebRequestClass = {
  .getQuery = requestGetQuery,
  .getCookie = requestGetCookie,
  .getHeader = requestGetHeader,
};

static int requestInit(struct MHDWebRequest* request, const struct MHDRequestParams* params, NQWebServer* server)
{
  NQWebRequest_init(&request->base);
  request->base.server = server;

  request->base.clazz = &kMHDWebRequestClass;
  request->base.userdata = NULL;
  request->base.url = request->url;
  request->base.method = request->method;
  request->base.version = request->version;

  strncpy(request->url, params->url, sizeof(request->url) - 1);
  strncpy(request->method, params->method, sizeof(request->method) - 1);
  strncpy(request->version, params->version, sizeof(request->version) - 1);

  request->queryParams = NQKeyVal_create();
  request->cookies = NQKeyVal_create();
  request->headers = NQKeyVal_create();

  return 0;
}

static void requestRelease(struct MHDWebRequest* request)
{
  NQKeyVal_destroy(request->queryParams);
  NQKeyVal_destroy(request->cookies);
  NQKeyVal_destroy(request->headers);

  NQWebRequest_finalize(&request->base);
}

static bool responseSetHeader(NQWebResponse* res, const char* header, const char* value)
{
  struct MHDWebResponse* response = NQ_CONTAINER_OF(res, struct MHDWebResponse, base);
  return NQKeyVal_set(response->headers, header, value);
}

static int responseWrite(NQWebResponse* res, const void* data, size_t size)
{
  struct MHDWebResponse* response = NQ_CONTAINER_OF(res, struct MHDWebResponse, base);
  return NQStringPrint_write(&response->buffer, (const char*)data, size);
}

static int responseFlush(NQWebResponse* response)
{
  return 0;
}

static const struct NQWebResponseOperations kResponseOperations = {
  .setHeader = responseSetHeader,
  .write = responseWrite,
  .flush = responseFlush,
};

static int responseInit(struct MHDWebResponse* response, NQWebRequest* request)
{
  NQWebResponse_init(&response->base, &kResponseOperations, request);

  response->headers = NQKeyVal_create();
  NQStringPrint_init(&response->buffer);
  return 0;
}

static void responseRelease(struct MHDWebResponse* response)
{
  NQKeyVal_destroy(response->headers);
  NQStringPrint_finalize(&response->buffer);

  NQWebResponse_finalize(&response->base);
}

static int websocketSend2(struct MHDWebSocket* thiz, const uint8_t* data, size_t size, uint8_t opcode, bool fin)
{
  if (size == 0)
    return 0;

  struct BufferEntry* entry = (struct BufferEntry*)NQMalloc(sizeof(*entry));
  if (entry == NULL)
    return -NQ_ENOMEM;

  entry->opcode = opcode;
  NQListHead_init(&entry->list);
  NQWebSocketBuffer_init(&entry->buffer);
  if (!NQWebSocketBuffer_reserve(&entry->buffer, size)) {
    NQFree((void*)entry);
    return -NQ_ENOMEM;
  }

  int len = NQWebSocketBuffer_write(&entry->buffer, data, size);
  NQ_ASSERT(len == size);

  NQWebSocketBuffer_complete(&entry->buffer, opcode, fin, NULL);
  len = NQWebSocketBuffer_size(&entry->buffer);

  NQMutex_lock(&thiz->mutex);
  NQListHead_addBack(&thiz->outputList, &entry->list);
  NQMutex_unlock(&thiz->mutex);

  NQNetworkLooper_wakeup(thiz->base.looper);
  return len;
}

static int websocketSend(NQWebSocket* sock, const uint8_t* data, size_t size, unsigned flags)
{
  struct MHDWebSocket* thiz = (struct MHDWebSocket*)sock;
  bool fin = (flags & WEB_WSBIT_FIN) != 0;
  uint8_t opcode = kNQWebSocketOpcodeBinary;
  if (flags & WEB_WSOPCODE_TEXT)
    opcode = kNQWebSocketOpcodeText;
  else if (flags & WEB_WSOPCODE_BINARY)
    opcode = kNQWebSocketOpcodeBinary;
  return websocketSend2(thiz, data, size, opcode, fin);
}

static void websocketClose(NQWebSocket* sock, uint16_t statusCode)
{
  struct MHDWebSocket* thiz = (struct MHDWebSocket*)sock;
  uint8_t data[2];
  NQPutUint16BE(data, statusCode);
  websocketSend2(thiz, data, sizeof(data), kNQWebSocketOpcodeConnectionClose, true);
}

static void websocketRelease(NQWebSocket* sock)
{
  struct MHDWebSocket* thiz = (struct MHDWebSocket*)sock;
  while (!NQListHead_isEmpty(&thiz->outputList)) {
    struct BufferEntry* entry = NQ_CONTAINER_OF(thiz->outputList.next, struct BufferEntry, list);
    NQListHead_remove(&entry->list);
    NQWebSocketBuffer_finalize(&entry->buffer);
    NQFree(entry);
  }
  NQFree(thiz);
}

static const struct NQWebSocketClass kMHDWebSocketClass = {
  .send = websocketSend,
  .close = websocketClose,
  .release = websocketRelease,
};

static int websocketAction(NQSocketHandle handle, int events, void* userdata)
{
  struct MHDWebSocket* thiz = (struct MHDWebSocket*)userdata;

  if (events == 0)
    /* do nothing */;
  else if (events & NQ_POLLERR) {
    NQWebSocket_doClose(&thiz->base, WEB_WSRES_POLL);
    return 0;
  }
  else if (events & NQ_POLLHUP) {
    NQWebSocket_doClose(&thiz->base, WEB_WSRES_HANG_UP);
    return 0;
  }
  else if (events & NQ_POLLNVAL) {
    NQWebSocket_doClose(&thiz->base, WEB_WSRES_NOT_OPEN);
    return 0;
  }

  bool isEmptyBuffer;
  if (events & NQ_POLLOUT) {
    struct BufferEntry* entry;

    NQMutex_lock(&thiz->mutex);
    NQ_ASSERT(!NQListHead_isEmpty(&thiz->outputList));
    entry = NQ_CONTAINER_OF(thiz->outputList.next, struct BufferEntry, list);
    NQListHead_remove(&entry->list);
    isEmptyBuffer = NQListHead_isEmpty(&thiz->outputList);
    NQMutex_unlock(&thiz->mutex);

    NQ_ASSERT(entry->buffer.payloadLen != 0);
    const uint8_t* data = NQWebSocketBuffer_data(&entry->buffer);
    size_t sz = NQWebSocketBuffer_size(&entry->buffer);
    while (sz != 0) {
      int ret = NQSocketSend(thiz->handle, data, sz, 0);
      if (0 > ret) {
  #ifdef NQ_OS_LINUX
        if (EAGAIN == errno) {
          continue;
        }
  #endif
        return false;
      }
      data += ret;
      sz -= (size_t)ret;
    }

    NQWebSocketBuffer_finalize(&entry->buffer);
    NQFree(entry);

    if (entry->opcode == kNQWebSocketOpcodeConnectionClose)
      NQSocketClose(thiz->handle);
  }
  else {
    if (events & NQ_POLLIN) {
      int ret = NQSocketRecv(handle, thiz->data, sizeof(thiz->data), 0);
      if (0 > ret) {
        NQWebSocket_doClose(&thiz->base, WEB_WSRES_UNKNOWN);
        return 0;
      }
      else if (0 == ret) {
        NQWebSocket_doClose(&thiz->base, WEB_WSRES_SUCCESS);
        return 0;
      }
      else {
        thiz->size = 0;
        NQWebSocketFrame frame;
        if (NQWebSocketFrameParse(thiz->data, (size_t)ret, &frame)) {
          NQWebSocket_doReceive(&thiz->base, frame.payload, frame.payloadSize, frame.opcode);
        }
        else {
          NQWebSocket_doClose(&thiz->base, WEB_WSRES_UNKNOWN);
          return 0;
        }
      }
    }

    NQMutex_lock(&thiz->mutex);
    isEmptyBuffer = NQListHead_isEmpty(&thiz->outputList);
    NQMutex_unlock(&thiz->mutex);
  }

  return isEmptyBuffer ? NQ_POLLIN : NQ_POLLIN | NQ_POLLOUT;
}

static void websocketDestroy(NQSocketHandle handle, void* userdata)
{
  struct MHDWebSocket* thiz = (struct MHDWebSocket*)userdata;
  MHD_upgrade_action(thiz->urh, MHD_UPGRADE_ACTION_CLOSE);
}

static void websocketUpgrade(void* cls, struct MHD_Connection* connection, void* conCls, const char* extraIn, size_t extraInSize, MHD_socket handle, struct MHD_UpgradeResponseHandle* urh)
{
  struct WebContext* context = (struct WebContext*)conCls;
  struct MHDWebSocket* thiz = NQ_CONTAINER_OF(cls, struct MHDWebSocket, base);

  thiz->extraIn = extraIn;
  thiz->extraInSize = extraInSize;
  thiz->handle = handle;
  thiz->urh = urh;

  NQWebSocket_doOpen(&thiz->base);
  if (!NQNetworkLooper_addSocket(thiz->base.looper, handle, websocketAction, websocketDestroy, thiz)) {
    NQWebSocket_doClose(&thiz->base, WEB_WSRES_NOMEMORY);
    MHD_upgrade_action(thiz->urh, MHD_UPGRADE_ACTION_CLOSE);
    NQWebSocket_release(&thiz->base); // FIXME
  }
}

static struct MHDWebSocket* websocketCreate(NQNetworkLooper* looper)
{
  struct MHDWebSocket* thiz = (struct MHDWebSocket*)NQMalloc(sizeof(struct MHDWebSocket));
  if (thiz == NULL)
    return NULL;

  thiz->base.clazz = &kMHDWebSocketClass;
  thiz->base.looper = looper;
  thiz->base.userdata = NULL;

  thiz->statusCode = WEB_WSRES_SUCCESS;
  thiz->size = 0;
  thiz->extraIn = NULL;
  thiz->extraInSize = 0;
  thiz->urh = NULL;

  NQMutex_init(&thiz->mutex);
  NQListHead_init(&thiz->outputList);

  return thiz;
}

static int websocketUpgradeInit(NQWebRequest* request, void* data)
{
  return 0;
}

static size_t websocketUpgradeReceive(NQWebRequest* request, const char* data, size_t size)
{
  return 0;
}

static int websocketUpgradeHandler(NQWebRequest* request, NQWebResponse* response)
{
  const char* secWsKeyHeader = NQWebRequest_getHeader(request, NQHTTP_HEADER_SEC_WEBSOCKET_KEY);
  if (secWsKeyHeader == NULL) {
    return NQ_HTTP_UPGRADE_REQUIRED;
  }

  const char* secWsVersionHeader = NQWebRequest_getHeader(request, NQHTTP_HEADER_SEC_WEBSOCKET_VERSION);
  if (secWsVersionHeader == NULL || NQStrcmp(secWsVersionHeader, WS_VERSION) != 0) {
    NQWebResponse_setHeader(response, NQHTTP_HEADER_SEC_WEBSOCKET_VERSION, WS_VERSION);
    return NQ_HTTP_UPGRADE_REQUIRED;
  }

#if 0
  const char* secWsProtocolHeader = NQWebRequest_getHeader(request, NQHTTP_HEADER_SEC_WEBSOCKET_PROTOCOL);
  if (secWsProtocolHeader == NULL) { // TODO: check protocol
    return NQ_HTTP_PRECONDITION_FAILED;
  }
#endif

  NQWebServer* server = NQWebRequest_server(request);
  NQNetworkLooper* looper = NQWebServer_looper(server);
  if (looper == NULL) {
    return NQ_HTTP_SERVICE_UNAVAILABLE;
  }

  struct MHDWebSocket* ws = websocketCreate(looper);
  if (ws == NULL)
    return NQ_HTTP_SERVICE_UNAVAILABLE;

  struct MHDWebContext* context = NQ_CONTAINER_OF(request, struct MHDWebContext, request.base);
  context->socket = &ws->base;
  if (!NQWebServer_initSocket(server, request, &ws->base)) {
    NQWebSocket_release(&ws->base);
    return NQ_HTTP_NOT_FOUND;
  }

  char hashedKey[32];
  NQWebSocketCalcAccept(secWsKeyHeader, hashedKey, sizeof(hashedKey));

  NQWebResponse_setHeader(response, NQHTTP_HEADER_UPGRADE, "websocket");
  // NQWebResponse_setHeader(response, NQHTTP_HEADER_CONNECTION, "Upgrade");
  NQWebResponse_setHeader(response, NQHTTP_HEADER_SEC_WEBSOCKET_ACCEPT, hashedKey);
  // NQWebResponse_setHeader(response, NQHTTP_HEADER_SEC_WEBSOCKET_PROTOCOL, secWsProtocolHeader);

  return NQ_HTTP_SWITCHING_PROTOCOLS;
}

static void websocketUpgradeRelease(NQWebRequest* request)
{
}

static const NQWebRequestOperations kWebSocketUpgradeOps = {
  .init = websocketUpgradeInit,
  .receive = websocketUpgradeReceive,
  .handler = websocketUpgradeHandler,
  .release = websocketUpgradeRelease,
};

static MHDWebResult keyValueIterator(void* cls, enum MHD_ValueKind kind, const char* key, const char* value)
{
  struct MHDWebRequest* request = (struct MHDWebRequest*)cls;

  if (value == NULL)
    value = NQCStrEmpty();

  if (kind == MHD_HEADER_KIND)
    NQKeyVal_set(request->headers, key, value);
  else if (kind == MHD_GET_ARGUMENT_KIND)
    NQKeyVal_set(request->queryParams, key, value);
  else if (kind == MHD_COOKIE_KIND)
    NQKeyVal_set(request->cookies, key, value);

  return MHD_YES;
}

static void contextRelease(struct MHDWebContext* thiz)
{
  requestRelease(&thiz->request);
  responseRelease(&thiz->response);
  NQFree(thiz);
}

static struct MHDWebContext* contextCreate(NQWebServer* server, const struct MHDRequestParams* params)
{
  struct MHDWebContext* thiz = (struct MHDWebContext*)NQZalloc(sizeof(*thiz));
  if (thiz == NULL) {
    return NULL;
  }

  thiz->connection = params->connection;

  requestInit(&thiz->request, params, server);

  MHD_get_connection_values(params->connection, MHD_HEADER_KIND, &keyValueIterator, &thiz->request);
  MHD_get_connection_values(params->connection, MHD_COOKIE_KIND, &keyValueIterator, &thiz->request);
  MHD_get_connection_values(params->connection, MHD_GET_ARGUMENT_KIND, &keyValueIterator, &thiz->request);

  responseInit(&thiz->response, &thiz->request.base);

  thiz->socket = NULL;

  const char* upgrageHeader = NQWebRequest_getHeader(&thiz->request.base, NQHTTP_HEADER_UPGRADE);
  if (upgrageHeader && NQStrcmp(upgrageHeader, "websocket") == 0) {
    const char* connectionHeader = NQWebRequest_getHeader(&thiz->request.base, NQHTTP_HEADER_CONNECTION);
    if (connectionHeader && NQStrcmp(connectionHeader, "Upgrade") == 0) {
      thiz->request.base.operations = &kWebSocketUpgradeOps;
      thiz->request.base.userdata = NULL;
      return thiz;
    }
  }

  if (!NQWebServer_initRequest(server, &thiz->request.base)) {
    contextRelease(thiz);
    return NULL;
  }

  return thiz;
}

static MHDWebResult serverAccessHandler(void* cls, struct MHD_Connection* connection,
                               const char* url, const char* method, const char* version,
                               const char* uploadDataPtr, size_t* uploadDataSize,
                               void** ptr)
{
  NQWebServer* thiz = (NQWebServer*)cls;
  struct MHDWebContext* context;

  unsigned int statusCode;

  if (*ptr == NULL) {
    struct MHDRequestParams params = {
      .connection = connection,
      .url = url,
      .method = method,
      .version = version,
    };
    context = contextCreate(thiz, &params);
    if (context == NULL) {
      return MHD_NO;
    }
    NQHttpStatistics_inc(thiz->statistics, NQWebRequest_method(&context->request.base), NQWebRequest_url(&context->request.base));
    *ptr = context;
    return MHD_YES;
  }

  context = (struct MHDWebContext*)(*ptr);

  if (uploadDataPtr && uploadDataSize) {
    if (context->request.base.operations->receive) {
      size_t n = context->request.base.operations->receive(&context->request.base, uploadDataPtr, *uploadDataSize);
      if (n < *uploadDataSize)
        *uploadDataSize -= n;
      else {
        *uploadDataSize = 0;
      }
    }
    else {
      *uploadDataSize = 0;
    }
    return MHD_YES;
  }

  statusCode = NQ_HTTP_OK;
  if (context->request.base.operations->handler) {
    statusCode = context->request.base.operations->handler(&context->request.base, &context->response.base);
    NQWebResponse_flush(&context->response.base);
  }

  struct MHD_Response* response;
  if (context->socket != NULL)
    response = MHD_create_response_for_upgrade(websocketUpgrade, context->socket);
  else {
    char* data = NQStringPrint_characters(&context->response.buffer);
    size_t size = NQStringPrint_length(&context->response.buffer);
    response = MHD_create_response_from_buffer(size, data, MHD_RESPMEM_PERSISTENT);
  }

  NQKeyValIter* iter = NQKeyVal_begin(context->response.headers);
  while (iter != NULL) {
    const char* key = NQKeyValIter_key(iter);
    const char* val = NQKeyValIter_val(iter);
    MHD_add_response_header(response, key, val);
    iter = NQKeyValIter_next(iter);
  }

  int ret = MHD_queue_response(context->connection, statusCode, response);
  MHD_destroy_response(response);

  return ret;
}

static void serverRequestCompleted(void* cls, struct MHD_Connection* connection, void** ptr, enum MHD_RequestTerminationCode code)
{
  NQWebServer* server = (NQWebServer*)(cls);
  struct MHDWebContext* context = (struct MHDWebContext*)(*ptr);
  if (context != NULL) {
    NQ_ASSERT(context->connection == connection);

    if (context->request.base.operations->release)
      context->request.base.operations->release(&context->request.base);

    contextRelease(context);
    *ptr = NULL;
  }
}

static bool serverInit(NQWebServer* thiz)
{
  thiz->userdata = NULL;
  return true;
}

struct MHDWebDaemonParams {
  uint16_t port;
  bool addressReuse;
  int connectionTimeout;
  const char* keyPEM;
  const char* certPEM;
};

static struct MHD_Daemon* createDaemon(NQWebServer* thiz, const struct MHDWebDaemonParams* params)
{
  unsigned int flags = 0;

  flags |= MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_POLL | MHD_ALLOW_UPGRADE;

#if !defined(NDEBUG)
  flags |= MHD_USE_DEBUG | MHD_USE_ERROR_LOG;
#endif

#define MHD_OPTION_LIST \
  NULL, NULL, \
  &serverAccessHandler, thiz, \
  MHD_OPTION_NOTIFY_COMPLETED, &serverRequestCompleted, thiz, \
  MHD_OPTION_LISTENING_ADDRESS_REUSE, params->addressReuse ? 1 : 0, \
  MHD_OPTION_CONNECTION_TIMEOUT, params->connectionTimeout

  if (params->keyPEM && params->certPEM) {
    flags |= MHD_USE_TLS;
    return MHD_start_daemon(flags, params->port, MHD_OPTION_LIST,
#ifdef HAVE_MHD_STRICT_FOR_CLIENT
                            MHD_OPTION_STRICT_FOR_CLIENT, NULL,
#endif
                            MHD_OPTION_HTTPS_MEM_KEY, params->keyPEM,
                            MHD_OPTION_HTTPS_MEM_CERT, params->certPEM,
                            MHD_OPTION_END);
  }

  return MHD_start_daemon(flags, params->port, MHD_OPTION_LIST,
#ifdef HAVE_MHD_STRICT_FOR_CLIENT
                            MHD_OPTION_STRICT_FOR_CLIENT, NULL,
#endif
                            MHD_OPTION_END);
}

static int serverStart(NQWebServer* thiz)
{
  struct MHDWebDaemonParams params;
  params.port = NQUrlHost_port(thiz->host);
  params.addressReuse = true;
  params.connectionTimeout = 120;
  params.keyPEM = NULL;
  params.certPEM = NULL;

  if (thiz->tlsEnabled) {
    params.keyPEM = NQWebServer_tlsKey(thiz);
    if (params.keyPEM == NULL)
      return -NQ_EINVAL;

    params.certPEM = NQWebServer_tlsCert(thiz);
    if (params.certPEM == NULL)
      return -NQ_EINVAL;
  }

  struct MHD_Daemon* daemon = createDaemon(thiz, &params);
  if (daemon == NULL)
    return -NQ_EIO;

  thiz->userdata = daemon;
  return 0;
}

static int serverStop(NQWebServer* thiz)
{
  struct MHD_Daemon* daemon = thiz->userdata;
  MHD_stop_daemon(daemon);
  thiz->userdata = NULL;
  return 0;
}

static void serverRelease(NQWebServer* thiz)
{
  struct MHD_Daemon* daemon = thiz->userdata;
  if (daemon != NULL)
    MHD_stop_daemon(daemon);
}

extern NQWebServerOperations kMHDServerOperations;
NQWebServerOperations kMHDServerOperations = {
  .name = "MHD",
  .init = serverInit,
  .start = serverStart,
  .stop = serverStop,
  .release = serverRelease,
};

#endif /* WITH_MHD */
