/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/web/WebEnvRestExecutor.h"

#include <libnetq/string/StringPrint.h>
#include <libnetq/string/String.h>
#include <libnetq/http/HttpHeader.h>
#include <libnetq/ByteBuffer.h>
#include <libnetq/json/JSONWriter.h>
#include <libnetq/http/MediaType.h>
#include <libnetq/web/JsonRpcTypes.h>
#include <libnetq/Env.h>
#include <libnetq/Event.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/Malloc.h>
#include <libnetq/web/WebRequest.h>
#include <libnetq/web/WebResponse.h>

static bool jsonWriterHandler(void* userdata, const char* characters, size_t size)
{
  NQWebResponse* response = (NQWebResponse*)userdata;
  int n = NQWebResponse_write(response, characters, size);
  return n < 0 ? false : true;
}

static int allEnvRequest(NQWebRequest* request, NQWebResponse* response)
{
  NQWebResponse_setHeader(response, NQHTTP_HEADER_CONTENT_TYPE, NQ_MEDIATYPE_APPLICATION_JSON);

  NQByteBuffer buf;
  NQByteBuffer_init(&buf);

  NQJSONWriter writer;
  NQJSONWriter_init(&writer, &jsonWriterHandler, response);
  NQJSONWriter_writeObjectBegin(&writer);

  NQEnviron* env = NQEnviron_create();
  if (env != NULL) {
    NQJSONWriter_writeKeyObjectBegin(&writer, NQ_JSONRPC_NAME_RESULT);
    for (const NQEnvironIter* iter = NQEnviron_begin(env); iter; iter = NQEnvironIter_next(iter)) {
      size_t len = NQEnvironIter_read(iter, (char*)NQByteBuffer_data(&buf), NQByteBuffer_size(&buf));
      if (len == 0)
        continue;

      if (NQByteBuffer_size(&buf) <= len) {
        if (!NQByteBuffer_resize(&buf, len + 1))
          return NQ_HTTP_INTERNAL_SERVER_ERROR;
        NQEnvironIter_read(iter, (char*)NQByteBuffer_data(&buf), NQByteBuffer_size(&buf));
      }

      char* key = (char*)NQByteBuffer_data(&buf);
      char* val = strchr(key, '=');
      if (val) {
        *val++ = '\0';
        NQJSONWriter_writeKeyString(&writer, key, val);
      }
    }
    NQJSONWriter_writeObjectEnd(&writer);
    NQEnviron_destroy(env);
  }
  else {
    NQJSONWriter_writeKeyObjectBegin(&writer, NQ_JSONRPC_NAME_ERROR);
    NQJSONWriter_writeKeyString(&writer, NQ_JSONRPC_NAME_MESSAGE, "Environ is unavailable");
    NQJSONWriter_writeObjectEnd(&writer);
  }

  NQJSONWriter_writeObjectEnd(&writer);
  NQJSONWriter_finalize(&writer);

  return env ? NQ_HTTP_OK : NQ_HTTP_INTERNAL_SERVER_ERROR;
}

static const NQWebRequestOperations kAllEnvOps = {
  .handler = allEnvRequest,
};

static int getEnvRequest(NQWebRequest* request, NQWebResponse* response)
{
  NQWebResponse_setHeader(response, NQHTTP_HEADER_CONTENT_TYPE, NQ_MEDIATYPE_APPLICATION_JSON);

  NQJSONWriter writer;
  NQJSONWriter_init(&writer, &jsonWriterHandler, response);

  const char* name = NQWebRequest_getSegment(request, "name");
  if (name == NULL)
    return NQ_HTTP_INTERNAL_SERVER_ERROR; // TODO: No default json error

  char buffer[64];
  int len = NQEnvGet(name, buffer, sizeof(buffer));

  NQJSONWriter_writeObjectBegin(&writer);
  if (len == -1) {
    NQJSONWriter_writeKeyObjectBegin(&writer, NQ_JSONRPC_NAME_RESULT);
    NQJSONWriter_writeKeyNull(&writer, name);
    NQJSONWriter_writeObjectEnd(&writer);
  }
  else {
    char* ptr;

    NQByteBuffer buf2;
    NQByteBuffer_init(&buf2);

    if (len < sizeof(buffer))
      ptr = buffer;
    else if (NQByteBuffer_resize(&buf2, len + 1))
      ptr = (char*)NQByteBuffer_data(&buf2);
    else
      ptr = NULL;

    if (ptr) {
      NQEnvGet(name, ptr, len + 1);
      NQJSONWriter_writeKeyObjectBegin(&writer, NQ_JSONRPC_NAME_RESULT);
      NQJSONWriter_writeKeyString2(&writer, name, ptr, len);
      NQJSONWriter_writeObjectEnd(&writer);
    }
    else {
      NQJSONWriter_writeKeyObjectBegin(&writer, NQ_JSONRPC_NAME_ERROR);
      NQJSONWriter_writeKeyString(&writer, NQ_JSONRPC_NAME_MESSAGE, "No Memory");
      NQJSONWriter_writeObjectEnd(&writer);
    }
    NQByteBuffer_finalize(&buf2);
  }
  NQJSONWriter_writeObjectEnd(&writer);
  NQJSONWriter_finalize(&writer);

  return NQ_HTTP_OK;
}

static const NQWebRequestOperations kGetEnvOps = {
  .handler = getEnvRequest,
};

static int setEnvRequest(NQWebRequest* request, NQWebResponse* response)
{
  return NQ_HTTP_NOT_IMPLEMENTED;
}

static const NQWebRequestOperations kSetEnvOps = {
  .handler = setEnvRequest,
};

int NQWebEnvRestListenersInit(NQWebExecutor* executor, NQWebEnvRestListeners* listeners, const NQWebEnvRestParams* params)
{
  if (params->baseUrl == NULL)
    return -NQ_EINVAL;

  int ret;
  ret = NQWebExecutor_addRequestListener(executor, &listeners->allEnvListener, &kAllEnvOps, NULL, NQ_HTTP_GET, "%s", params->baseUrl);
  if (ret)
    return ret;

  ret = NQWebExecutor_addRequestListener(executor, &listeners->getEnvListener, &kGetEnvOps, NULL, NQ_HTTP_GET, "%s/{name}", params->baseUrl);
  if (ret) {
    NQWebExecutor_removeRequestListener(executor, &listeners->allEnvListener);
    return ret;
  }

  ret = NQWebExecutor_addRequestListener(executor, &listeners->setEnvListener, &kSetEnvOps, NULL, NQ_HTTP_POST, "%s/{name}", params->baseUrl);
  if (ret) {
    NQWebExecutor_removeRequestListener(executor, &listeners->getEnvListener);
    NQWebExecutor_removeRequestListener(executor, &listeners->allEnvListener);
    return ret;
  }

  return 0;
}

void NQWebEnvRestListenersFinalize(NQWebExecutor* executor, NQWebEnvRestListeners* listeners)
{
  struct NQWebEnvRestExecutor* envApi = (NQWebEnvRestExecutor*)executor;
  NQWebExecutor_removeRequestListener(&envApi->executor, &listeners->setEnvListener);
  NQWebExecutor_removeRequestListener(&envApi->executor, &listeners->getEnvListener);
  NQWebExecutor_removeRequestListener(&envApi->executor, &listeners->allEnvListener);
}

static int executorInit(NQWebExecutor* executor, void* data)
{
  struct NQWebEnvRestParams* params = (struct NQWebEnvRestParams*)data;
  struct NQWebEnvRestExecutor* envApi = (NQWebEnvRestExecutor*)executor;
  return NQWebEnvRestListenersInit(&envApi->executor, &envApi->listeners, params);
}

static void executorRelease(NQWebExecutor* executor)
{
  struct NQWebEnvRestExecutor* envApi = (NQWebEnvRestExecutor*)executor;
  NQWebEnvRestListenersFinalize(&envApi->executor, &envApi->listeners);
}

static const struct NQWebExecutorOperations kWebRestEnvOps = {
  .init = executorInit,
  .release = executorRelease,
};

NQWebEnvRestExecutor* NQWebEnvRestExecutorCreate(NQWebServer* server, struct NQWebEnvRestParams* params)
{
  return (NQWebEnvRestExecutor*)NQWebServer_createExecutor(server, sizeof(struct NQWebEnvRestExecutor), &kWebRestEnvOps, (void*)params);
}

void NQWebEnvRestExecutorDestroy(NQWebServer* server, NQWebEnvRestExecutor* restApi)
{
  NQWebServer_destroyExecutor(server, &restApi->executor);
}
