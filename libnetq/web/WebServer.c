/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/web/WebServer.h"

#define NQ_LOG_TAG "NQWebServer"

#include <libnetq/string/StringUtil.h>
#include <libnetq/CType.h>
#include <libnetq/Malloc.h>
#include <libnetq/Limits.h>
#include <libnetq/Log.h>
#include <libnetq/fs/Path.h>
#include <libnetq/Network.h>
#include <libnetq/http/HttpHeader.h>
#include <libnetq/asset/FileSystemAsset.h>
#include <libnetq/URL.h>
#include <libnetq/UrlPath.h>
#include <libnetq/http/MediaType.h>
#include <libnetq/Assert.h>
#include <libnetq/MinMax.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/random/CryptoRandom.h>
#include <libnetq/web/WebRequest.h>
#include <libnetq/web/WebResponse.h>
#include <libnetq/web/WebSocket.h>
#include <libnetq/Context.h>
#include <libnetq/Mutex.h> // For NQWebExecutorRegister

struct WebWriterEntry {
  const struct NQWebWriterOperations* operations;
  void* userdata;
  NQListHead list;
  char contentType[1];
};

enum MatchType {
  kMatchText,
  kMatchSegments,
  kMatchPattern,
  kMatchAnyBefore,
  kMatchAny,
};

static NQ_LISTHEAD_DEFINE(g_registredOpsList);

#if defined(WITH_MHD)
extern NQWebServerOperations kMHDServerOperations;
#endif

#if defined(WITH_CIVETWEB)
extern NQWebServerOperations kCivetWebServerOperations;
#endif

static NQWebServerOperations* g_builtinOpsList[] = {
#if defined(WITH_MHD)
  &kMHDServerOperations,
#endif

#if defined(WITH_CIVETWEB)
  &kCivetWebServerOperations,
#endif

  NULL
};

static const NQWebServerOperations* defaultOperations(void)
{
  if (!NQListHead_isEmpty(&g_registredOpsList))
    return NQ_CONTAINER_OF(g_registredOpsList.next, struct NQWebServerOperations, list);
  if (*g_builtinOpsList)
    return *g_builtinOpsList;
  return NULL;
}

NQWebServer* NQWebServer_create(const NQWebServerParams* params)
{
  NQWebServer* thiz = (NQWebServer*)NQZalloc(sizeof(NQWebServer));
  if (NQWebServer_init(thiz, params, NULL))
    return thiz;
  NQWebServer_finalize(thiz);
  NQFree((void*)thiz);
  return NULL;
}

void NQWebServer_destroy(NQWebServer* thiz)
{
  NQWebServer_finalize(thiz);
  NQFree((void*)thiz);
}

static NQString* loadFileAsString(const char* workDir, const char* filename)
{
  NQPathBuilder pathBld;
  NQPathBuilder_init(&pathBld);
  if (!NQPathBuilder_resolve2(&pathBld, workDir, filename)) {
    NQ_LOGE("Unable to resolve path %s", filename);
    NQPathBuilder_finalize(&pathBld);
    return NULL;
  }

  NQString* result = NQString_fromFile(NQPathBuilder_characters(&pathBld));
  if (result == NULL) {
    NQ_LOGE("Unable to load %s", NQPathBuilder_characters(&pathBld));
  }

  NQPathBuilder_finalize(&pathBld);
  return result;
}

bool NQWebServer_init(NQWebServer* thiz, const NQWebServerParams* params, NQWebServerSupervisor* parent)
{
  thiz->parent = parent;
  if (parent && parent->serverOps)
    thiz->operations = parent->serverOps;
  else {
    thiz->operations = defaultOperations();
    if (thiz->operations == NULL) {
      NQ_LOGE("No server implementation available");
      return false;
    }
  }

  thiz->looper = thiz->parent ? thiz->parent->looper : NULL;
  if (thiz->looper == NULL) {
    thiz->looper = NQNetworkLooper_create(1024, 1024, 1024);
    if (thiz->looper == NULL) {
      NQ_LOGE("Unable to create looper");
      return false;
    }
  }

  thiz->host = NQUrlHost_create(params->host);
  if (thiz->host == NULL) {
    if (thiz->parent == NULL || thiz->parent->looper != thiz->looper)
      NQNetworkLooper_destroy(thiz->looper);
    return false;
  }

  NQStringData_init(&thiz->workDir);
  NQStringData_set(&thiz->workDir, params->workDir);

  NQStringData_init(&thiz->resourceDir);
  NQStringData_set(&thiz->resourceDir, params->resourceDir);

  thiz->tlsEnabled = params->tlsEnabled;
  if (!NQUrlHost_hasPort(thiz->host)) {
     NQUrlHost_setPort(thiz->host, thiz->tlsEnabled ? NQ_DEFAULT_HTTPS_PORT : NQ_DEFAULT_HTTP_PORT);
  }

  thiz->tlsKeyString = NQCStrIsNullOrEmpty(params->tlsKey) ? NULL : loadFileAsString(params->workDir, params->tlsKey);
  thiz->tlsCertString = NQCStrIsNullOrEmpty(params->tlsCert) ? NULL : loadFileAsString(params->workDir, params->tlsCert);
  thiz->asset = NQCStrIsNullOrEmpty(params->resourceDir) ? NULL : NQFileSystemAssetCreate(params->resourceDir);

  NQListHead_init(&thiz->executors);
  NQListHead_init(&thiz->listeners);
  NQListHead_init(&thiz->writerExecutors);
  NQListHead_init(&thiz->moduleList);
  NQListHead_init(&thiz->catalogEntries);
  thiz->statistics = NQHttpStatistics_create();
  NQPrimitiveStorage_init(&thiz->storage, thiz->parent ? thiz->parent->storage : NULL);

  NQGetCryptoRandom(thiz->sessionSeckey, sizeof(thiz->sessionSeckey));

  thiz->mimetypes = NQKeyVal_create();
  thiz->isLooperRunning = false;

  thiz->userdata = NULL;
  return thiz->operations->init(thiz);
}

void NQWebExecutor_init(NQWebExecutor* thiz, const struct NQWebExecutorOperations* operations)
{
  thiz->server = NULL;
  thiz->userdata = NULL;
  thiz->operations = operations;
  NQListHead_init(&thiz->list);
}

NQWebExecutor* NQWebExecutor_alloc(size_t sizeInBytes, const struct NQWebExecutorOperations* mops)
{
  struct NQWebExecutor* thiz = (struct NQWebExecutor*)NQZalloc(NQGetMax(sizeof(*thiz), sizeInBytes));
  if (thiz == NULL)
    return NULL;

  NQWebExecutor_init(thiz, mops);
  return thiz;
}

void NQWebExecutor_release(NQWebExecutor* thiz)
{
  NQ_ASSERT(NQListHead_isEmpty(&thiz->list));
  NQFree(thiz);
}

static void moduleEntryRelease(NQWebServer* thiz, NQWebExecutor* entry)
{
  if (entry->operations && entry->operations->release)
    entry->operations->release(entry);
  if (entry->release)
    entry->release(entry);
  NQListHead_remove(&entry->list);
  NQWebExecutor_release(entry);
}

void NQWebServer_finalize(NQWebServer* thiz)
{
  thiz->operations->release(thiz);

  NQPrimitiveStorage_finalize(&thiz->storage);
  NQHttpStatistics_destroy(thiz->statistics);

  {
    NQListHead* iter = thiz->writerExecutors.next;
    while (iter != &thiz->writerExecutors) {
      struct WebWriterEntry* entry = NQ_CONTAINER_OF(iter, struct WebWriterEntry, list);
      NQListHead* next = entry->list.next;
      NQListHead_remove(&entry->list);
      NQFree(entry);
      iter = next;
    }
  }

  while (!NQListHead_isEmpty(&thiz->moduleList)) {
    NQWebExecutor* entry = NQ_CONTAINER_OF(thiz->moduleList.next, struct NQWebExecutor, list);
    moduleEntryRelease(thiz, entry);
  }

  while (!NQListHead_isEmpty(&thiz->catalogEntries)) {
    struct NQWebCatalogEntry* entry = NQ_CONTAINER_OF(thiz->catalogEntries.next, struct NQWebCatalogEntry, serverList);
    NQ_LOGE("The catalog entry '%s' was not removed", entry->params.mainUrl);
    NQListHead_remove(&entry->serverList);
  }

  if (thiz->mimetypes)
    NQKeyVal_release(thiz->mimetypes);

  while (!NQListHead_isEmpty(&thiz->executors)) {
    NQWebExecutor* entry = NQ_CONTAINER_OF(thiz->executors.next, struct NQWebExecutor, list);
    NQListHead_remove(&entry->list);
    NQFree(entry);
  }

  if (thiz->asset != NULL)
    NQAsset_destroy(thiz->asset);

  NQStringData_finalize(&thiz->workDir);
  NQStringData_finalize(&thiz->resourceDir);

  if (thiz->tlsKeyString != NULL)
    NQString_release(thiz->tlsKeyString);

  if (thiz->tlsCertString != NULL)
    NQString_release(thiz->tlsCertString);

  if (thiz->parent == NULL || thiz->parent->looper != thiz->looper)
    NQNetworkLooper_destroy(thiz->looper);
}

static bool comparePattern(const char* pattern, const char* url)
{
  char a = *pattern++;
  if (a == '\0')
    return false;

  for (;;) {
    char b = *url++;
    if (a == b) {
      if (a == '\0')
        return true;
      NQ_ASSERT(b != '*');
      a = *(pattern++);
      continue;
    }
    if (a != '*')
      return false;
    if (pattern[0] == '\0')
      return true;
    if (comparePattern(pattern, url))
      return true;
  }
}

bool NQWebServer_initRequest(NQWebServer* thiz, NQWebRequest* request, NQWebSocket* sock)
{
  NQListHead* head = &thiz->listeners;
  const char* url = NQWebRequest_url(request);
  const char* method = NQWebRequest_method(request);

  NQ_ASSERT(!request->urlPath);
  NQUrlPath* urlPath = NQUrlPath_create(url, NULL, false);
  if (urlPath == NULL)
    return false;

  NQListHead* iter;
  for (iter = head->next; iter != head; iter = iter->next) {
    struct NQWebRequestListener* entry = NQ_CONTAINER_OF(iter, struct NQWebRequestListener, list);
    if (sock == NULL && entry->operations == NULL)
      continue;
    if (sock != NULL && entry->operations != NULL)
      continue;

    if (NQStrcmp(entry->method, method) != 0)
      continue;

    switch (entry->patternKind) {
    case kMatchText:
      if (NQStrcmp(entry->pattern, url) != 0)
        continue;
      request->urlPath = urlPath;
      break;

    case kMatchSegments:
      request->urlPath = NQUrlPath_create(url, entry->pattern, true);
      if (request->urlPath == NULL)
        continue;
      break;

    case kMatchPattern:
      if (!comparePattern(entry->pattern, url))
        continue;
      request->urlPath = urlPath;
      break;

    case kMatchAnyBefore:
    case kMatchAny:
      request->urlPath = urlPath;
      break;

    default:
      continue;
    }

    if (entry->operations != NULL) {
      request->operations = entry->operations;
      request->userdata = entry->userdata;
      if (request->operations->init == NULL || request->operations->init(request, request->userdata) == 0) {
        if (request->urlPath != urlPath)
          NQUrlPath_destroy(urlPath);
        return true;
      }
      request->operations = NULL;
      request->userdata = NULL;
    }
    else {
      struct NQWebSocketListener* socketListener = NQ_CONTAINER_OF(entry, struct NQWebSocketListener, requestListener);
      sock->operations = socketListener->operations;
      sock->userdata = socketListener->userdata;
      if (sock->operations->init == NULL || sock->operations->init(sock, sock->userdata) == 0) {
        if (request->urlPath != urlPath)
          NQUrlPath_destroy(urlPath);
        return true;
      }
    }

    if (request->urlPath != urlPath)
      NQUrlPath_destroy(request->urlPath);
    request->urlPath = NULL;
  }

  NQUrlPath_destroy(urlPath);
  return false;
}

static int getPatternKind(const char* pattern)
{
  if (!NQStrcmp(pattern, "*:before"))
    return kMatchAnyBefore;
  else if (!NQStrcmp(pattern, "*"))
    return kMatchAny;
  else if (strchr(pattern, '*') != NULL)
    return kMatchPattern;
  else if (NQIsUrlPathPattern(pattern))
    return kMatchSegments;
  else
    return kMatchText;
}

static int addRequestListener(NQWebServer* thiz, struct NQWebRequestListener* entry)
{
  if (NQCStrIsNullOrEmpty(entry->method) || NQCStrIsNullOrEmpty(entry->pattern))
    return -NQ_EINVAL;

  entry->patternKind = getPatternKind(entry->pattern);

  NQListHead* iter = thiz->listeners.next;
  for (;;) {
    if (iter == &thiz->listeners) {
      NQListHead_addBack(&thiz->listeners, &entry->list);
      break;
    }
    struct NQWebRequestListener* it = NQ_CONTAINER_OF(iter, struct NQWebRequestListener, list);
    if (it->patternKind > entry->patternKind) {
      NQListHead_addBack(&it->list, &entry->list);
      break;
    }
    iter = iter->next;
  }

  return 0;
}

static void removeRequestListener(NQWebServer* thiz, struct NQWebRequestListener* listener)
{
  NQ_ASSERT(!NQListHead_isEmpty(&listener->list));
  if (listener->patternKind == kMatchText)
    // TODO: NQWebServer_removeMetric();
  NQListHead_remove(&listener->list);
}

int NQWebExecutor_addRequestListener(NQWebExecutor* executor, struct NQWebRequestListener* listener, const NQWebRequestOperations* operations, void* userdata, const char* method, const char* format, ...)
{
  va_list args;
  va_start(args, format);
  char* newPattern = NQCStrFormatV(format, args);
  va_end(args);

  if (newPattern == NULL)
    return -NQ_ENOMEM;

  char* newMethod = NQCStrDuplicate(method);
  if (newMethod == NULL) {
    NQCStrFree(newPattern);
    return -NQ_ENOMEM;
  }

  listener->method = newMethod;
  listener->pattern = newPattern;
  listener->executor = executor;
  NQListHead_init(&listener->list);
  listener->userdata = userdata;
  listener->operations = operations;

  int ret = addRequestListener(executor->server, listener);
  if (ret) {
    NQCStrFree(newMethod);
    NQCStrFree(newPattern);
  }
  else if (listener->patternKind == kMatchText) {
    NQWebServer_allowMetric(executor->server, listener->method, listener->pattern);
  }

  return ret;
}

void NQWebExecutor_removeRequestListener(NQWebExecutor* executor, struct NQWebRequestListener* listener)
{
  removeRequestListener(executor->server, listener);
  NQCStrFree(listener->method);
  NQCStrFree(listener->pattern);
}

static void removeSocketListener(NQWebServer* thiz, struct NQWebSocketListener* executor)
{
  NQListHead_remove(&executor->requestListener.list);
}

int NQWebExecutor_addSocketListener(NQWebExecutor* executor, struct NQWebSocketListener* listener, const NQWebSocketOperations* operations, void* userdata, const char* method, const char* format, ...)
{
  va_list args;
  va_start(args, format);
  char* newPattern = NQCStrFormatV(format, args);
  va_end(args);

  if (newPattern == NULL)
    return -NQ_ENOMEM;

  char* newMethod = NQCStrDuplicate(method);
  if (newMethod == NULL) {
    NQCStrFree(newMethod);
    return -NQ_ENOMEM;
  }

  listener->requestListener.method = newMethod;
  listener->requestListener.pattern = newPattern;
  listener->requestListener.executor = executor;
  NQListHead_init(&listener->requestListener.list);
  listener->requestListener.userdata = NULL;
  listener->requestListener.operations = NULL;
  listener->userdata = userdata;
  listener->operations = operations;

  int ret = addRequestListener(executor->server, &listener->requestListener);
  if (ret) {
    NQCStrFree(newMethod);
    NQCStrFree(newPattern);
  }

  return ret;
}

void NQWebExecutor_removeSocketListener(NQWebExecutor* executor, struct NQWebSocketListener* listener)
{
  removeSocketListener(executor->server, listener);
  NQCStrFree(listener->requestListener.method);
  NQCStrFree(listener->requestListener.pattern);
}

struct NQWebRequestExecutor {
  NQWebExecutor executor;
  size_t listenerCount;
  struct NQWebRequestListener listeners[1];
};

static void releaseRequestExecutor(NQWebExecutor* executor)
{
  struct NQWebRequestExecutor* execApi = NQ_CONTAINER_OF(executor, struct NQWebRequestExecutor, executor);
  for (size_t i = 0; i < execApi->listenerCount; i++)
    NQWebExecutor_removeRequestListener(&execApi->executor, &execApi->listeners[i]);
}

NQWebExecutor* NQWebServer_createRequestExecutor(NQWebServer* thiz, const char* method, const char* url, const NQWebRequestOperations* operations, void* data)
{
  const NQWebRequestMatch matches[] = { { .method = method, .url = url }, { NULL } };
  return NQWebServer_createRequestExecutorEx(thiz, matches, operations, data);
}

NQWebExecutor* NQWebServer_createRequestExecutorEx(NQWebServer* thiz, const NQWebRequestMatch* matches, const NQWebRequestOperations* operations, void* data)
{
  size_t listenerCount = 0;
  for (const NQWebRequestMatch* iter = matches; iter->method && iter->url; iter = &matches[listenerCount])
    listenerCount++;
  if (listenerCount == 0)
    return NULL;

  struct NQWebRequestExecutor* execApi;
  size_t sizeInBytes = sizeof(struct NQWebRequestExecutor) - sizeof(execApi->listeners) + sizeof(*execApi->listeners) * listenerCount;
  execApi = (struct NQWebRequestExecutor*)NQWebServer_createExecutor(thiz, sizeInBytes, NULL, NULL);
  if (execApi == NULL)
    return NULL;

  for (size_t index = 0; index < listenerCount; index++) {
    int ret = NQWebExecutor_addRequestListener(&execApi->executor, &execApi->listeners[index], operations, data, matches[index].method, "%s", matches[index].url);
    if (ret) {
      for (size_t i = 0; i < index; i++)
        NQWebExecutor_removeRequestListener(&execApi->executor, &execApi->listeners[i]);
      NQWebServer_destroyExecutor(thiz, &execApi->executor);
      return NULL;
    }
  }

  execApi->listenerCount = listenerCount;
  execApi->executor.release = releaseRequestExecutor;
  return &execApi->executor;
}

static int onBlobInit(NQWebRequest* request, void* data)
{
  request->userdata = data;
  return 0;
}

static int onBlobHandler(NQWebRequest* request, NQWebResponse* response)
{
  NQWebBlob* blob = (NQWebBlob*)request->userdata;

  if (blob->type) {
    NQWebResponse_setHeader(response, NQHTTP_HEADER_CONTENT_TYPE, blob->type);
  }

  (void)NQWebResponse_write(response, blob->data, blob->size);
  return NQ_HTTP_OK;
}

static const NQWebRequestOperations kBlobOps = {
  .init = onBlobInit,
  .handler = onBlobHandler,
};

NQWebExecutor* NQWebServer_createRequestBlob(NQWebServer* thiz, const char* method, const char* url, const NQWebBlob* blob)
{
  return NQWebServer_createRequestExecutor(thiz, method, url, &kBlobOps, (void*)blob);
}

NQWebExecutor* NQWebServer_createRequestBlobEx(NQWebServer* thiz, const NQWebRequestMatch* matches, const NQWebBlob* blob)
{
  return NQWebServer_createRequestExecutorEx(thiz, matches, &kBlobOps, (void*)blob);
}

struct NQWebSocketExecutor {
  NQWebExecutor executor;
  size_t listenerCount;
  struct NQWebSocketListener listeners[1];
};

static void releaseSocketExecutor(NQWebExecutor* pexec)
{
  struct NQWebSocketExecutor* execApi = NQ_CONTAINER_OF(pexec, struct NQWebSocketExecutor, executor);
  for (size_t i = 0; i < execApi->listenerCount; i++)
    NQWebExecutor_removeSocketListener(&execApi->executor, &execApi->listeners[i]);
}

NQWebExecutor* NQWebServer_createSocketExecutor(NQWebServer* thiz, const char* method, const char* url, const NQWebSocketOperations* operations, void* data)
{
  const NQWebRequestMatch matches[] = { { .method = method, .url = url }, { NULL } };
  return NQWebServer_createSocketExecutorEx(thiz, matches, operations, data);
}

NQWebExecutor* NQWebServer_createSocketExecutorEx(NQWebServer* thiz, const NQWebRequestMatch* matches, const NQWebSocketOperations* operations, void* data)
{
  size_t listenerCount = 0;
  for (const NQWebRequestMatch* iter = matches; iter->method && iter->url; iter = &matches[listenerCount])
    listenerCount++;
  if (listenerCount == 0)
    return NULL;

  struct NQWebSocketExecutor* execApi;
  size_t sizeInBytes = sizeof(struct NQWebSocketExecutor) - sizeof(execApi->listeners) + sizeof(*execApi->listeners) * listenerCount;
  execApi = (struct NQWebSocketExecutor*)NQWebServer_createExecutor(thiz, sizeInBytes, NULL, NULL);
  if (execApi == NULL)
    return NULL;

  for (size_t index = 0; index < listenerCount; index++) {
    int ret = NQWebExecutor_addSocketListener(&execApi->executor, &execApi->listeners[index], operations, data, matches[index].method, "%s", matches[index].url);
    if (ret) {
      for (size_t i = 0; i < index; i++)
        NQWebExecutor_removeSocketListener(&execApi->executor, &execApi->listeners[i]);
      NQWebServer_destroyExecutor(thiz, &execApi->executor);
      return NULL;
    }
  }

  execApi->listenerCount = listenerCount;
  execApi->executor.release = releaseSocketExecutor;
  return &execApi->executor;
}

bool NQWebServer_registerWriter(NQWebServer* thiz, const char* contentType, const struct NQWebWriterOperations* operations, void* userdata)
{
  size_t len = NQStrlen(contentType);
  struct WebWriterEntry* entry = (struct WebWriterEntry*)NQMalloc(sizeof(*entry) + len);
  if (entry == NULL)
    return false;

  entry->operations = operations;
  entry->userdata = userdata;
  memcpy(entry->contentType, contentType, len + 1);
  NQListHead_addBack(&thiz->writerExecutors, &entry->list);

  return true;
}

void NQWebServer_unregisterWriter(NQWebServer* thiz, const struct NQWebWriterOperations* operations, void* userdata)
{
  NQListHead* iter = thiz->writerExecutors.next;
  while (iter != &thiz->writerExecutors) {
    struct WebWriterEntry* entry = NQ_CONTAINER_OF(iter, struct WebWriterEntry, list);
    if (entry->operations == operations) {
      NQListHead_remove(&entry->list);
      NQFree(entry);
      break;
    }
    iter = iter->next;
  }
}

struct NQWebWriter* NQWebServer_createWriterChain(NQWebServer* thiz, const char* contentType, NQWebRequest* request)
{
  struct NQWebWriter* result = NULL;
  NQListHead* iter = thiz->writerExecutors.prev;
  while (iter != &thiz->writerExecutors) {
    struct WebWriterEntry* entry = NQ_CONTAINER_OF(iter, struct WebWriterEntry, list);
    if (!strcmp(entry->contentType, contentType)) {
      struct NQWebWriter* writer = (struct NQWebWriter*)NQMalloc(sizeof(*writer));
      if (writer == NULL) {
        break;
      }

      writer->operations = entry->operations;
      writer->userdata = entry->userdata;
      writer->request = request;
      writer->next = NULL;

      if (writer->operations->init == NULL || writer->operations->init(writer) == 0) {
        writer->next = result;
        result = writer;
      }
      else {
        NQFree(writer);
      }
    }
    iter = iter->prev;
  }

  return result;
}

bool NQWebServer_allowMetric(NQWebServer* thiz, const char* method, const char* url)
{
  return NQIsUrlPath(url) && NQHttpStatistics_add(thiz->statistics, method, url);
}

NQUint8Array* NQWebServer_loadAssetBytes(const NQWebServer* thiz, const char* filename)
{
  if (thiz->asset == NULL) {
    return NULL;
  }

  return NQAssetFile_loadBytes(thiz->asset, filename);
}

const char* NQWebServer_getMimeType(const NQWebServer* thiz, const char* filename)
{
  const char* extension = strrchr(filename, '.');
  if (extension == NULL)
    return NQ_MEDIATYPE_APPLICATION_OCTETSTREAM;

  if (thiz->mimetypes != NULL) {
    const char* contentType = NQKeyVal_get(thiz->mimetypes, extension);
    if (contentType != NULL)
      return contentType;
  }

  if (thiz->parent != NULL && thiz->parent->mimetypes != NULL) {
    const char* contentType = NQKeyVal_get(thiz->parent->mimetypes, extension);
    if (contentType != NULL)
      return contentType;
  }

  return NQ_MEDIATYPE_APPLICATION_OCTETSTREAM;
}

bool NQWebServer_addMimeType(NQWebServer* thiz, const char* mimetype, const char* extname)
{
  return NQKeyVal_set(thiz->mimetypes, extname, mimetype);
}

struct NQWebCatalogEntry* NQWebCatalogEntryCreate(const NQWebCatalogParams* params)
{
  NQ_ASSERT(params->mainUrl != NULL);

  struct NQWebCatalogEntry* entry;
  size_t sizeInBytes = sizeof(*entry);

  size_t mainUrlLenz = NQStrlen(params->mainUrl) + 1;
  sizeInBytes += mainUrlLenz;
  size_t titleLenz = params->title ? NQStrlen(params->title) + 1 : 0;
  sizeInBytes += titleLenz;
  size_t versionLenz = params->version ? NQStrlen(params->version) + 1 : 0;
  sizeInBytes += versionLenz;
  size_t descriptionLenz = params->description ? NQStrlen(params->description) + 1 : 0;
  sizeInBytes += descriptionLenz;
  size_t lightIconUrlLenz = params->lightIconUrl ? NQStrlen(params->lightIconUrl) + 1 : 0;
  sizeInBytes += lightIconUrlLenz;
  size_t darkIconUrlLenz = params->darkIconUrl ? NQStrlen(params->darkIconUrl) + 1 : 0;
  sizeInBytes += darkIconUrlLenz;
  size_t lightScreenshotUrlLenz = params->lightScreenshotUrl ? NQStrlen(params->lightScreenshotUrl) + 1 : 0;
  sizeInBytes += lightScreenshotUrlLenz;
  size_t darkScreenshotUrlLenz = params->darkScreenshotUrl ? NQStrlen(params->darkScreenshotUrl) + 1 : 0;
  sizeInBytes += darkScreenshotUrlLenz;

  entry = (struct NQWebCatalogEntry*)NQZalloc(sizeInBytes);
  if (entry == NULL)
    return NULL;

  NQListHead_init(&entry->serverList);
  NQListHead_init(&entry->executorList);

  char* ptr = (char*)entry + sizeof(*entry);

  entry->params.mainUrl = ptr;
  memcpy(ptr, params->mainUrl, mainUrlLenz);
  ptr += mainUrlLenz;

  if (titleLenz) {
    entry->params.title = ptr;
    memcpy(ptr, params->title, titleLenz);
    ptr += titleLenz;
  }

  if (versionLenz) {
    entry->params.version = ptr;
    memcpy(ptr, params->version, versionLenz);
    ptr += versionLenz;
  }

  if (descriptionLenz) {
    entry->params.description = ptr;
    memcpy(ptr, params->description, descriptionLenz);
    ptr += descriptionLenz;
  }

  if (lightIconUrlLenz) {
    entry->params.lightIconUrl = ptr;
    memcpy(ptr, params->lightIconUrl, lightIconUrlLenz);
    ptr += lightIconUrlLenz;
  }

  if (darkIconUrlLenz) {
    entry->params.darkIconUrl = ptr;
    memcpy(ptr, params->darkIconUrl, darkIconUrlLenz);
    ptr += darkIconUrlLenz;
  }

  if (lightScreenshotUrlLenz) {
    entry->params.lightScreenshotUrl = ptr;
    memcpy(ptr, params->lightScreenshotUrl, lightScreenshotUrlLenz);
    ptr += lightScreenshotUrlLenz;
  }

  if (darkScreenshotUrlLenz) {
    entry->params.darkScreenshotUrl = ptr;
    memcpy(ptr, params->darkScreenshotUrl, darkScreenshotUrlLenz);
    ptr += darkScreenshotUrlLenz;
  }

  NQ_ASSERT((char*)entry + sizeInBytes == ptr);
  return entry;
}

void NQWebCatalogEntryDestroy(struct NQWebCatalogEntry* entry)
{
  NQ_ASSERT(NQListHead_isEmpty(&entry->serverList));
  NQ_ASSERT(NQListHead_isEmpty(&entry->executorList));
  NQFree(entry);
}

int NQWebServer_addCatalogEntry(NQWebServer* thiz, struct NQWebCatalogEntry* entry)
{
  NQListHead_addBack(&thiz->catalogEntries, &entry->serverList);
  return 0;
}

void NQWebServer_removeCatalogEntry(NQWebServer* thiz, struct NQWebCatalogEntry* entry)
{
  NQListHead_remove(&entry->serverList);
}

int NQWebServer_start(NQWebServer* thiz)
{
  return thiz->operations->start(thiz);
}

int NQWebServer_stop(NQWebServer* thiz)
{
  return thiz->operations->stop(thiz);
}

static void doWork(NQWebServer* thiz)
{
  while (thiz->isLooperRunning) {
    while (NQNetworkLooper_performOnce(thiz->looper)) {
      if (!thiz->isLooperRunning)
        return;
    }
    if (NQNetworkLooper_poll(thiz->looper, 500) < 0) {
      NQ_LOGE("Looper poll failed");
      return;
    }
  }
}

int NQWebServer_run(NQWebServer* thiz)
{
  if (thiz->parent != NULL && thiz->parent->looper == thiz->looper)
    return -NQ_EIO;

  thiz->isLooperRunning = true;
  int ret = thiz->operations->start(thiz);
  if (ret == 0) {
    doWork(thiz);
    thiz->operations->stop(thiz);
  }
  thiz->isLooperRunning = false;

  return ret;
}

static int initExecutor(NQWebServer* thiz, size_t sizeInBytes, const struct NQWebExecutorOperations* operations, void* data, NQWebExecutor** result)
{
  NQWebExecutor* executor = NQWebExecutor_alloc(sizeInBytes, operations);
  executor->server = thiz;
  executor->release = NULL;
  NQListHead_addBack(&thiz->moduleList, &executor->list);

  if (executor->operations && executor->operations->init) {
    int ret = executor->operations->init(executor, data);
    if (ret != 0) {
      NQListHead_remove(&executor->list);
      NQWebExecutor_release(executor);
      return ret;
    }
  }

  if (result)
    *result = executor;

  return 0;
}

NQWebExecutor* NQWebServer_createExecutor(NQWebServer* thiz, size_t sizeInBytes, const struct NQWebExecutorOperations* operations, void* data)
{
  NQWebExecutor* executor;
  int ret = initExecutor(thiz, sizeInBytes, operations, data, &executor);
  return ret ? NULL : executor;
}

void NQWebServer_destroyExecutor(NQWebServer* thiz, NQWebExecutor* executor)
{
  NQ_ASSERT(thiz == executor->server);
  moduleEntryRelease(thiz, executor);
}

int NQWebServer_loadExecutor(NQWebServer* thiz, const char* name)
{
  const struct NQWebExecutorOperations* operations = NQWebExecutorFind(name);
  if (!operations)
    return -NQ_ENOENT;
  return initExecutor(thiz, operations->size, operations, NULL, NULL);
}

int NQWebServer_loadExecutorWLA(NQWebServer* thiz, const char* name)
{
  int ret = NQWebServer_loadExecutor(thiz, name);
  if (ret == 0)
    return 0;

  if (ret != -NQ_ENOENT)
    return ret;

  ret = NQContext_loadModule(NQContext_instance(), "%s_wla", name);
  if (ret != 0)
    return ret;

  return NQWebServer_loadExecutor(thiz, name);
}

void NQWebServerOperationsRegister(NQWebServerOperations* operations)
{
  NQListHead_addBack(&g_registredOpsList, &operations->list);
}

void NQWebServerOperationsUnregister(NQWebServerOperations* operations)
{
  NQListHead_remove(&operations->list);
}

static NQ_MUTEX_DEFINE(s_executorMutex);
static NQ_LISTHEAD_DEFINE(s_executorList);

struct NQWebExecutorOperations* NQWebExecutorFind(const char* name)
{
  NQListHead* iter;
  struct NQWebExecutorOperations* result = NULL;

  NQMutex_lock(&s_executorMutex);
  for (iter = s_executorList.next; iter != &s_executorList; iter = iter->next) {
    struct NQWebExecutorOperations* ops = NQ_CONTAINER_OF(iter, struct NQWebExecutorOperations, list);
    if (!NQStrcmp(ops->name, name)) {
      result = ops;
      break;
    }
  }
  NQMutex_unlock(&s_executorMutex);

  return result;
}

int NQWebExecutorRegister(struct NQWebExecutorOperations* ops)
{
  NQMutex_lock(&s_executorMutex);
  NQListHead_addBack(&s_executorList, &ops->list);
  NQMutex_unlock(&s_executorMutex);
  return 0;
}

void NQWebExecutorUnregister(struct NQWebExecutorOperations* ops)
{
  NQMutex_lock(&s_executorMutex);
  NQListHead_remove(&ops->list);
  NQMutex_unlock(&s_executorMutex);
}
