/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/web/WebDirAccess.h"

#include <libnetq/Log.h>
#include <libnetq/List.h>
#include <libnetq/Malloc.h>
#include <libnetq/fs/Path.h>
#include <libnetq/http/HttpHeader.h>
#include <libnetq/web/WebRequest.h>
#include <libnetq/web/WebResponse.h>
#include <libnetq/ErrorCode.h>

struct NQWebDirAccessApi {
  NQWebExecutor executor;
  struct NQWebRequestListener listener;
  NQPath* baseDir;
  NQPath* baseUrl;
};

static int requestInit(NQWebRequest* request, void* data)
{
  struct NQWebDirAccessApi* restApi = (struct NQWebDirAccessApi*)data;

  const char* url = NQWebRequest_url(request);
  if (!NQUrlPathStartsWith(url, NQPath_characters(restApi->baseUrl)))
    return -NQ_EINVAL;

  const char* relativePath = url + NQPath_length(restApi->baseUrl) + 1;

  NQPathBuilder pathBld;
  NQPathBuilder_init(&pathBld);

  if (!NQPathBuilder_join2(&pathBld, NQPath_characters(restApi->baseDir), relativePath)) {
    NQPathBuilder_finalize(&pathBld);
    return -NQ_ENOMEM;
  }

  if (!NQPathStartsWith(NQPathBuilder_characters(&pathBld), NQPath_characters(restApi->baseDir))) {
    NQPathBuilder_finalize(&pathBld);
    return -NQ_EINVAL;
  }

  NQUint8Array* arraybuffer = NQUint8Array_fromFile(NQPathBuilder_characters(&pathBld));
  if (arraybuffer == NULL) {
    NQPathBuilder_finalize(&pathBld);
    return -NQ_ENOENT;
  }

  request->userdata = arraybuffer;
  NQPathBuilder_finalize(&pathBld);
  return 0;
}

static int requestHandler(NQWebRequest* request, NQWebResponse* response)
{
  NQUint8Array* arraybuffer = (NQUint8Array*)request->userdata;

  const char* url = NQWebRequest_url(request);
  NQWebServer* server = NQWebRequest_server(request);
  const char* contentType = NQWebServer_getMimeType(server, url);

  NQWebResponse_setHeader(response, NQHTTP_HEADER_CONTENT_TYPE, contentType);
  NQWebResponse_write(response, NQUint8Array_data(arraybuffer), NQUint8Array_size(arraybuffer));
  return NQ_HTTP_OK;
}

static void requestRelease(NQWebRequest* request)
{
  NQUint8Array* arraybuffer = (NQUint8Array*)request->userdata;
  NQUint8Array_destroy(arraybuffer);
}

static const NQWebRequestOperations kDirAccessOps = {
  .init = requestInit,
  .handler = requestHandler,
  .release = requestRelease,
};

static int restApiInit(NQWebExecutor* executor, void* data)
{
  struct NQWebDirAccessApi* restApi = NQ_CONTAINER_OF(executor, struct NQWebDirAccessApi, executor);
  const struct NQWebDirAccessParams* params = (const struct NQWebDirAccessParams*)data;
  if (params->baseDir == NULL || params->baseUrl == NULL)
    return -NQ_EINVAL;

  if (!NQIsAbsolutePath(params->baseDir) || !NQIsAbsolutePosixPath(params->baseUrl))
    return -NQ_EINVAL;

  restApi->baseDir = NQPath_resolve1(params->baseDir);
  if (restApi->baseDir == NULL)
    return -NQ_ENOMEM;

  restApi->baseUrl = NQPath_join1(params->baseUrl);
  if (restApi->baseUrl == NULL) {
    NQPath_destroy(restApi->baseDir);
    return -NQ_ENOMEM;
  }

  int ret = NQWebExecutor_addRequestListener(&restApi->executor, &restApi->listener, &kDirAccessOps, restApi, NQ_HTTP_GET, "%s/*", NQPath_characters(restApi->baseUrl));
  if (ret != 0) {
    NQPath_destroy(restApi->baseUrl);
    NQPath_destroy(restApi->baseDir);
    return ret;
  }

  return 0;
}

static void restApiRelease(NQWebExecutor* executor)
{
  struct NQWebDirAccessApi* restApi = NQ_CONTAINER_OF(executor, struct NQWebDirAccessApi, executor);
  NQWebExecutor_removeRequestListener(&restApi->executor, &restApi->listener);
  NQPath_destroy(restApi->baseUrl);
  NQPath_destroy(restApi->baseDir);
}

static const struct NQWebExecutorOperations kWebDirAccessOps = {
  .init = restApiInit,
  .release = restApiRelease,
};

NQWebDirAccessApi* NQWebDirAccessCreate(NQWebServer* server, const struct NQWebDirAccessParams* params)
{

  return (NQWebDirAccessApi*)NQWebServer_createExecutor(server, sizeof(struct NQWebDirAccessApi), &kWebDirAccessOps, (void*)params);
}

void NQWebDirAccessDestroy(NQWebServer* server, NQWebDirAccessApi* restApi)
{
  NQWebServer_destroyExecutor(server, &restApi->executor);
}
