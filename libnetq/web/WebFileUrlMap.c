/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/web/WebFileUrlMap.h"

#include <libnetq/Log.h>
#include <libnetq/List.h>
#include <libnetq/Malloc.h>
#include <libnetq/fs/Path.h>
#include <libnetq/http/HttpHeader.h>
#include <libnetq/web/WebRequest.h>
#include <libnetq/web/WebResponse.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/Assert.h>

struct NQWebFileUrlMapEntry {
  NQListHead list;
  struct NQWebRequestListener listener;
  NQPath* url;
  NQPath* file;
};

struct NQWebFileUrlMapApi {
  NQWebExecutor executor;
  NQListHead listeners;
};

static int fileUrlMapInit(NQWebRequest* request, void* data)
{
  struct NQWebFileUrlMapEntry* entry = (struct NQWebFileUrlMapEntry*)data;
  request->userdata = entry->file;
  return 0;
}

static int fileUrlMapRequest(NQWebRequest* request, NQWebResponse* response)
{
  NQPath* filename = (NQPath*)NQWebRequest_userdata(request);

  NQUint8Array* arraybuffer = NQUint8Array_fromFile(NQPath_characters(filename));
  if (arraybuffer == NULL) {
    NQ_LOGE("Cannot load file '%s'", NQPath_characters(filename));
    return NQ_HTTP_INTERNAL_SERVER_ERROR;
  }

  const char* contentType = NQWebServer_getMimeType(NQWebRequest_server(request), NQPath_characters(filename));
  NQWebResponse_setHeader(response, NQHTTP_HEADER_CONTENT_TYPE, contentType);
  NQWebResponse_write(response, (char*)NQUint8Array_data(arraybuffer), NQUint8Array_size(arraybuffer));
  NQUint8Array_destroy(arraybuffer);
  return NQ_HTTP_OK;
}

static const NQWebRequestOperations kFileUrlMapOps = {
  .init = fileUrlMapInit,
  .handler = fileUrlMapRequest,
};

static void fileUrlMapApiFinalize(struct NQWebFileUrlMapApi* restApi)
{
  while (!NQListHead_isEmpty(&restApi->listeners)) {
    struct NQWebFileUrlMapEntry* entry = NQ_CONTAINER_OF(restApi->listeners.next, struct NQWebFileUrlMapEntry, list);
    NQWebExecutor_removeRequestListener(&restApi->executor, &entry->listener);
    NQListHead_remove(&entry->list);
    NQPath_destroy(entry->file);
    NQPath_destroy(entry->url);
    NQFree(entry);
  }
}

static int restApiInit(NQWebExecutor* executor, void* data)
{
  struct NQWebFileUrlMapApi* restApi = NQ_CONTAINER_OF(executor, struct NQWebFileUrlMapApi, executor);
  const struct NQWebFileUrlMapParams* params = (const struct NQWebFileUrlMapParams*)data;
  if (params->items == NULL)
    return -NQ_EINVAL;

  const char* baseDir;
  if (params->baseDir == NULL)
    baseDir = NQCStrEmpty();
  else if (NQIsAbsolutePath(params->baseDir))
    baseDir = params->baseDir;
  else
    return -NQ_EINVAL;

  const char* baseUrl;
  if (params->baseUrl == NULL)
    baseUrl = NQCStrEmpty();
  else if (NQIsAbsolutePath(params->baseUrl))
    baseUrl = params->baseUrl;
  else
    return -NQ_EINVAL;

  NQListHead_init(&restApi->listeners);

  uint32_t listenerCount = 0;
  while (true) {
    struct NQWebFileUrlMapItem* item = &params->items[listenerCount];
    if (item->file == NULL)
      break;

    const char* url = item->url;
    if (url == NULL) {
      if (NQIsAbsolutePath(item->file)) {
        fileUrlMapApiFinalize(restApi);
        return -NQ_EINVAL;
      }
      url = item->file;
    }

    struct NQWebFileUrlMapEntry* entry = (struct NQWebFileUrlMapEntry*)NQMalloc(sizeof(*entry));
    if (entry == NULL) {
      fileUrlMapApiFinalize(restApi);
      return -NQ_ENOMEM;
    }

    entry->file = NQPath_resolve2(baseDir, item->file);
    if (entry->file == NULL) {
      NQFree(entry);
      fileUrlMapApiFinalize(restApi);
      return -NQ_EINVAL;
    }

    entry->url = NQPath_resolve2(baseUrl, url);
    if (entry->url == NULL) {
      NQPath_destroy(entry->file);
      NQFree(entry);
      fileUrlMapApiFinalize(restApi);
      return -NQ_EINVAL;
    }

    int ret = NQWebExecutor_addRequestListener(&restApi->executor, &entry->listener, &kFileUrlMapOps, entry, NQ_HTTP_GET, "%s", NQPath_characters(entry->url));
    if (ret) {
      NQPath_destroy(entry->file);
      NQPath_destroy(entry->url);
      NQFree(entry);
      fileUrlMapApiFinalize(restApi);
      return ret;
    }

    listenerCount++;
    NQListHead_addBack(&restApi->listeners, &entry->list);
  }

  if (listenerCount == 0) {
    fileUrlMapApiFinalize(restApi);
    return -NQ_EINVAL;
  }

  return 0;
}

static void restApiRelease(NQWebExecutor* executor)
{
  struct NQWebFileUrlMapApi* restApi = NQ_CONTAINER_OF(executor, struct NQWebFileUrlMapApi, executor);
  fileUrlMapApiFinalize(restApi);
}

static const struct NQWebExecutorOperations kWebFileUrlMapOps = {
  .init = restApiInit,
  .release = restApiRelease,
};

NQWebFileUrlMapApi* NQWebFileUrlMapCreate(NQWebServer* server, const struct NQWebFileUrlMapParams* params)
{
  return (NQWebFileUrlMapApi*)NQWebServer_createExecutor(server, sizeof(struct NQWebFileUrlMapApi), &kWebFileUrlMapOps, (void*)params);
}

void NQWebFileUrlMapDestroy(NQWebServer* server, NQWebFileUrlMapApi* restApi)
{
  NQWebServer_destroyExecutor(server, &restApi->executor);
}
