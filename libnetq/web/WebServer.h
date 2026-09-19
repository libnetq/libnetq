/*
 * MIT License
 *
 * Copyright (c) 2023-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_WEB_WEBSERVER_H
#define _LIBNETQ_WEB_WEBSERVER_H

#include <libnetq/NetworkLooper.h>
#include <libnetq/Asset.h>
#include <libnetq/Array.h>
#include <libnetq/string/StringData.h>
#include <libnetq/string/StringRef.h>
#include <libnetq/http/HttpStatus.h>
#include <libnetq/http/HttpMethod.h>
#include <libnetq/http/HttpStatistics.h>
#include <libnetq/PrimitiveStorage.h>
#include <libnetq/web/UrlHost.h>
#include <libnetq/json/JSON.h>
#include <libnetq/KeyVal.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQWebRequest NQWebRequest;
typedef struct NQWebResponse NQWebResponse;
typedef struct NQWebSocket NQWebSocket;
typedef struct NQWebServer NQWebServer;

typedef struct NQWebRequestOperations NQWebRequestOperations;
typedef struct NQWebSocketOperations NQWebSocketOperations;
typedef struct NQWebWriterOperations NQWebWriterOperations;

typedef struct NQWebRequestMatch NQWebRequestMatch;
struct NQWebRequestMatch {
  const char* method;
  const char* url;
};

typedef struct NQWebExecutor NQWebExecutor;
struct NQWebExecutor {
  NQWebServer* server;
  NQListHead list;
  void (*release) (NQWebExecutor*);
  const struct NQWebExecutorOperations* operations;
  void* userdata;
};

struct NQWebExecutorOperations {
  NQListHead list;
  const char* name;
  int  (*init)    (NQWebExecutor*, void* data);
  void (*release) (NQWebExecutor*);
  const size_t size;
};

NQ_EXPORT struct NQWebExecutorOperations* NQWebExecutorFind(const char* name);
NQ_EXPORT int NQWebExecutorRegister(struct NQWebExecutorOperations* ops);
NQ_EXPORT void NQWebExecutorUnregister(struct NQWebExecutorOperations* ops);

NQ_EXPORT void NQWebExecutor_init(NQWebExecutor*, const struct NQWebExecutorOperations* operations);
NQ_EXPORT NQWebExecutor* NQWebExecutor_alloc(size_t sizeInBytes, const struct NQWebExecutorOperations* operations);
NQ_EXPORT void NQWebExecutor_release(NQWebExecutor*);

struct NQWebRequestListener {
  NQWebExecutor* executor;
  NQListHead list;

  const char* method;
  const char* pattern;
  int patternKind;

  const NQWebRequestOperations* operations;
  void* userdata;
};

struct NQWebSocketListener {
  struct NQWebRequestListener requestListener;
  const NQWebSocketOperations* operations;
  void* userdata;
};

typedef struct NQWebServerParams NQWebServerParams;
struct NQWebServerParams {
  const char* host;

  bool tlsEnabled;
  const char* tlsKey;
  const char* tlsCert;

  const char* workDir;
  const char* resourceDir;
};

typedef struct NQWebBlob NQWebBlob;
struct NQWebBlob {
  const char* type;
  const void* data;
  uint32_t size;
};

typedef struct NQWebCatalogParams NQWebCatalogParams;
struct NQWebCatalogParams {
  const char* mainUrl;
  const char* title;
  const char* version;
  const char* description;

  const char* lightIconUrl;
  const char* darkIconUrl;

  const char* lightScreenshotUrl;
  const char* darkScreenshotUrl;
};

struct NQWebCatalogEntry {
  NQListHead serverList;
  NQListHead executorList;
  NQWebCatalogParams params;
};

typedef struct NQWebServerOperations NQWebServerOperations;
struct NQWebServerOperations {
  NQListHead list;
  const char* name;
  bool   (*init)     (NQWebServer*);
  int    (*start)    (NQWebServer*);
  int    (*stop)     (NQWebServer*);
  void   (*release)  (NQWebServer*);
};

typedef struct NQWebServerSupervisor NQWebServerSupervisor;
struct NQWebServerSupervisor {
  NQPrimitiveStorage* storage;
  NQNetworkLooper* looper;
  const NQWebServerOperations* serverOps;
  NQKeyVal* mimetypes;
};

struct NQWebServer {
  NQWebServerSupervisor* parent;
  NQPrimitiveStorage storage;
  NQListHead executors;
  NQListHead listeners;
  NQListHead writerExecutors;
  NQListHead moduleList;
  NQListHead catalogEntries;
  NQHttpStatistics* statistics;
  NQUrlHost* host;
  NQStringData workDir;
  NQStringData resourceDir;
  NQString* tlsKeyString;
  NQString* tlsCertString;
  bool tlsEnabled;
  NQNetworkLooper* looper;
  volatile bool isLooperRunning;
  NQAsset* asset;
  NQKeyVal* mimetypes;
  uint8_t sessionSeckey[32];

  const struct NQWebServerOperations* operations;
  void* userdata;
};

NQ_EXPORT NQWebServer* NQWebServer_create(const NQWebServerParams* params);
NQ_EXPORT bool NQWebServer_init(NQWebServer*, const NQWebServerParams* params, NQWebServerSupervisor* parent);
NQ_EXPORT void NQWebServer_destroy(NQWebServer*);
NQ_EXPORT void NQWebServer_finalize(NQWebServer*);

NQ_EXPORT int NQWebServer_start(NQWebServer*);
NQ_EXPORT int NQWebServer_stop(NQWebServer*);
NQ_EXPORT int NQWebServer_run(NQWebServer*);

static inline const char* NQWebServer_workDir(const NQWebServer* thiz)
{
  return NQStringData_characters(&thiz->workDir);
}

static inline const char* NQWebServer_resourceDir(const NQWebServer* thiz)
{
  return NQStringData_characters(&thiz->resourceDir);
}

static inline bool NQWebServer_tlsEnabled(const NQWebServer* thiz)
{
  return thiz->tlsEnabled;
}

static inline char* NQWebServer_tlsKey(const NQWebServer* thiz)
{
  return NQString_characters(thiz->tlsKeyString);
}

static inline const char* NQWebServer_tlsCert(const NQWebServer* thiz)
{
  return NQString_characters(thiz->tlsCertString);
}

NQ_EXPORT int NQWebExecutor_addRequestListener(NQWebExecutor*, struct NQWebRequestListener*, const NQWebRequestOperations* operations, void* userdata, const char* method, const char* format, ...);
NQ_EXPORT void NQWebExecutor_removeRequestListener(NQWebExecutor*, struct NQWebRequestListener*);

NQ_EXPORT int NQWebExecutor_addSocketListener(NQWebExecutor*, struct NQWebSocketListener*, const NQWebSocketOperations* operations, void* userdata, const char* method, const char* format, ...);
NQ_EXPORT void NQWebExecutor_removeSocketListener(NQWebExecutor*, struct NQWebSocketListener*);

NQ_EXPORT NQWebExecutor* NQWebServer_createRequestBlob(NQWebServer*, const char* method, const char* url, const NQWebBlob* blob);
NQ_EXPORT NQWebExecutor* NQWebServer_createRequestBlobEx(NQWebServer*, const NQWebRequestMatch* matches, const NQWebBlob* blob);
NQ_EXPORT NQWebExecutor* NQWebServer_createRequestExecutor(NQWebServer*, const char* method, const char* url, const NQWebRequestOperations* operations, void* data);
NQ_EXPORT NQWebExecutor* NQWebServer_createRequestExecutorEx(NQWebServer*, const NQWebRequestMatch* matches, const NQWebRequestOperations* operations, void* data);
NQ_EXPORT NQWebExecutor* NQWebServer_createSocketExecutor(NQWebServer*, const char* method, const char* url, const NQWebSocketOperations* operations, void* data);
NQ_EXPORT NQWebExecutor* NQWebServer_createSocketExecutorEx(NQWebServer*, const NQWebRequestMatch* matches, const NQWebSocketOperations* operations, void* data);

NQ_EXPORT struct NQWebWriter* NQWebServer_createWriterChain(NQWebServer*, const char* contentType, NQWebRequest* request);
NQ_EXPORT bool NQWebServer_registerWriter(NQWebServer*, const char* contentType, const struct NQWebWriterOperations* operations, void* userdata);
NQ_EXPORT void NQWebServer_unregisterWriter(NQWebServer*, const struct NQWebWriterOperations* operations, void* userdata);

NQ_EXPORT bool NQWebServer_allowMetric(NQWebServer*, const char* method, const char* url);

NQ_EXPORT NQWebExecutor* NQWebServer_createExecutor(NQWebServer*, size_t sizeInBytes, const struct NQWebExecutorOperations* operations, void* data);
NQ_EXPORT void NQWebServer_destroyExecutor(NQWebServer*, NQWebExecutor* executor);
NQ_EXPORT int NQWebServer_loadExecutor(NQWebServer*, const char* name);
NQ_EXPORT int NQWebServer_loadExecutorWLA(NQWebServer*, const char* name);

static inline NQHttpStatistics* NQWebServer_statistics(NQWebServer* thiz)
{
  return thiz->statistics;
}

static inline NQPrimitiveStorage* NQWebServer_storage(NQWebServer* thiz)
{
  return &thiz->storage;
}

static inline NQUrlHost* NQWebServer_host(NQWebServer* thiz)
{
  return thiz->host;
}

static inline NQNetworkLooper* NQWebServer_looper(NQWebServer* thiz)
{
  return thiz->looper;
}

static inline NQAsset* NQWebServer_asset(const NQWebServer* thiz)
{
  return thiz->asset;
}

NQ_EXPORT NQUint8Array* NQWebServer_loadAssetBytes(const NQWebServer*, const char* filename);
NQ_EXPORT const char* NQWebServer_getMimeType(const NQWebServer*, const char* filename);
NQ_EXPORT bool NQWebServer_addMimeType(NQWebServer*, const char* mimetype, const char* extname);

NQ_EXPORT struct NQWebCatalogEntry* NQWebCatalogEntryCreate(const NQWebCatalogParams* params);
NQ_EXPORT void NQWebCatalogEntryDestroy(struct NQWebCatalogEntry*);
NQ_EXPORT int NQWebServer_addCatalogEntry(NQWebServer*, struct NQWebCatalogEntry* entry);
NQ_EXPORT void NQWebServer_removeCatalogEntry(NQWebServer*, struct NQWebCatalogEntry* entry);

NQ_EXPORT bool NQWebServer_initRequest(NQWebServer*, NQWebRequest* request, NQWebSocket* sock);

NQ_EXPORT void NQWebServerOperationsRegister(NQWebServerOperations*);
NQ_EXPORT void NQWebServerOperationsUnregister(NQWebServerOperations*);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_WEB_WEBSERVER_H */
