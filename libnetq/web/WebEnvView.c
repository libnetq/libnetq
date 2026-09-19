/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/web/WebEnvView.h"

#include <libnetq/string/StringPrint.h>
#include <libnetq/string/String.h>
#include <libnetq/http/HttpHeader.h>
#include <libnetq/ByteBuffer.h>
#include <libnetq/http/MediaType.h>
#include <libnetq/web/JsonRpcTypes.h>
#include <libnetq/Env.h>
#include <libnetq/Event.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/URL.h>
#include <libnetq/Assert.h>
#include <libnetq/Malloc.h>
#include <libnetq/web/WebRequest.h>
#include <libnetq/web/WebResponse.h>

struct NQWebEnvView {
  NQWebExecutor executor;
  struct NQWebRequestListener mainListener;
  struct NQWebRequestListener setterListener;
  char* baseUrl;
};

static int mainInit(NQWebRequest* request, void* data)
{
  struct NQWebEnvView* envView = NQ_CONTAINER_OF(data, struct NQWebEnvView, executor);
  request->userdata = envView;
  return 0;
}

static int mainHandler(NQWebRequest* request, NQWebResponse* response)
{
  struct NQWebEnvView* envView = (struct NQWebEnvView*)request->userdata;
  NQWebResponse_setHeader(response, NQHTTP_HEADER_CONTENT_TYPE, NQ_MEDIATYPE_TEXT_HTML);

  NQByteBuffer buf;
  NQByteBuffer_init(&buf);

  NQEnviron* env = NQEnviron_create();
  if (env == NULL)
    return NQ_HTTP_INTERNAL_SERVER_ERROR;

  NQWebResponse_printf(response, "<!DOCTYPE html>");
  NQWebResponse_printf(response, "<html>");
  NQWebResponse_printf(response, "<head>");
  NQWebResponse_printf(response, "<meta charset=\"UTF-8\">");
  NQWebResponse_printf(response, "<title>User Environment</title>");
  NQWebResponse_printf(response, "</head>");
  NQWebResponse_printf(response, "<body>");
  NQWebResponse_printf(response, "<h1>Server Environment</h1>");
  NQWebResponse_printf(response, "<hr><table>");
  NQWebResponse_printf(response, "<tbody>");

  for (const NQEnvironIter* iter = NQEnviron_begin(env); iter; iter = NQEnvironIter_next(iter)) {
    size_t len = NQEnvironIter_read(iter, (char*)NQByteBuffer_data(&buf), NQByteBuffer_size(&buf));
    if (len == 0)
      continue;

    if (NQByteBuffer_size(&buf) <= len) {
      if (!NQByteBuffer_resize(&buf, len + 1)) {
        NQEnviron_destroy(env);
        return NQ_HTTP_INTERNAL_SERVER_ERROR;
      }
      NQEnvironIter_read(iter, (char*)NQByteBuffer_data(&buf), NQByteBuffer_size(&buf));
    }

    char* key = (char*)NQByteBuffer_data(&buf);
    char* val = strchr(key, '=');
    if (val) {
      *val++ = '\0';
      NQWebResponse_printf(response, "<tr>");
      NQWebResponse_printf(response, "<td>%s</td>", key);
      NQWebResponse_printf(response, "<td>");
      NQWebResponse_printf(response, "<form id=\"%s\" method=\"POST\" action=\"%s\">", key, envView->baseUrl);
      NQWebResponse_printf(response, "<input type=\"text\" name=\"%s\" value=\"%s\">", key, val);
      NQWebResponse_printf(response, "</form>");
      NQWebResponse_printf(response, "</td>");
      NQWebResponse_printf(response, "<td><button type=\"submit\" form=\"%s\">Save</button></td>", key);
      NQWebResponse_printf(response, "</tr>");
    }
  }

  NQByteBuffer_finalize(&buf);

  NQWebResponse_printf(response, "</tbody>");
  NQWebResponse_printf(response, "<table/><hr>");
  NQWebResponse_printf(response, "</body>");
  NQWebResponse_printf(response, "</html>");
  NQEnviron_destroy(env);

  return NQ_HTTP_OK;
}

static const NQWebRequestOperations kMainRequestOps = {
  .init = mainInit,
  .handler = mainHandler,
};

struct SetterContext {
  bool success;
  NQStringPrint buffer;
  char* baseUrl;
};

static int setterInit(NQWebRequest* request, void* data)
{
  struct NQWebEnvView* envView = NQ_CONTAINER_OF(data, struct NQWebEnvView, executor);
  struct SetterContext* ctx = NQMalloc(sizeof(struct SetterContext));
  if (ctx != NULL) {
    ctx->success = true;
    NQStringPrint_init(&ctx->buffer);
    ctx->baseUrl = envView->baseUrl;
  }
  request->userdata = ctx;
  return 0;
}

static size_t setterReceive(NQWebRequest* request, const char* data, size_t size)
{
  struct SetterContext* ctx = (struct SetterContext*)request->userdata;
  if (ctx != NULL && ctx->success)
    ctx->success = NQStringPrint_writeAll(&ctx->buffer, data, size);
  return size;
}

static int setterHandler(NQWebRequest* request, NQWebResponse* response)
{
  struct SetterContext* ctx = (struct SetterContext*)request->userdata;
  if (ctx == NULL || ctx->success == false) {
    return NQ_HTTP_INTERNAL_SERVER_ERROR;
  }

  int n;
  size_t length;
  char* delimiter;
  char* name = NQStringPrint_characters(&ctx->buffer);
  for (;;) {
    delimiter = strchr(name, '=');
    if (delimiter == NULL)
      return NQ_HTTP_BAD_REQUEST;

    length = delimiter - name;
    n = NQURLDecode(name, length, name, length + 1);
    if (n < 0 || n > length)
      return NQ_HTTP_BAD_REQUEST;
    NQ_ASSERT(name[n] == '\0');

    char* value = delimiter + 1;
    delimiter = strchr(value, '&');
    length = delimiter ? delimiter - value : NQStrlen(value);
    n = NQURLDecode(value, length, value, length + 1);
    if (n < 0 || n > length)
      return NQ_HTTP_BAD_REQUEST;
    NQ_ASSERT(value[n] == '\0');

    n = NQEnvSet(name, value);
    if (n < 0)
      return NQ_HTTP_INTERNAL_SERVER_ERROR;

    if (delimiter == NULL)
      break;

    name = value + length + 1;
  }

  NQWebResponse_setHeader(response, NQHTTP_HEADER_LOCATION, ctx->baseUrl);
  return NQ_HTTP_MOVED_TEMPORARILY;
}

static void setterRelease(NQWebRequest* request)
{
  struct SetterContext* ctx = (struct SetterContext*)request->userdata;
  if (ctx != NULL)
    NQStringPrint_finalize(&ctx->buffer);
  NQFree(ctx);
}

static const NQWebRequestOperations kSetterRequestOps = {
  .init = setterInit,
  .receive = setterReceive,
  .handler = setterHandler,
  .release = setterRelease,
};

static int executorInit(NQWebExecutor* executor, void* data)
{
  int ret;

  struct NQWebEnvViewParams* params = (struct NQWebEnvViewParams*)data;
  struct NQWebEnvView* envView = NQ_CONTAINER_OF(executor, struct NQWebEnvView, executor);

  envView->baseUrl = NQCStrDuplicate(params->baseUrl);
  if (envView->baseUrl == NULL) {
    return -NQ_ENOMEM;
  }

  ret = NQWebExecutor_addRequestListener(&envView->executor, &envView->mainListener, &kMainRequestOps, envView, NQ_HTTP_GET, "%s", params->baseUrl);
  if (ret) {
    NQCStrFree(envView->baseUrl);
    return ret;
  }

  ret = NQWebExecutor_addRequestListener(&envView->executor, &envView->setterListener, &kSetterRequestOps, envView, NQ_HTTP_POST, "%s", params->baseUrl);
  if (ret) {
    NQWebExecutor_removeRequestListener(&envView->executor, &envView->mainListener);
    NQCStrFree(envView->baseUrl);
    return ret;
  }

  return 0;
}

static void executorRelease(NQWebExecutor* executor)
{
  struct NQWebEnvView* envView = NQ_CONTAINER_OF(executor, struct NQWebEnvView, executor);

  NQWebExecutor_removeRequestListener(&envView->executor, &envView->setterListener);
  NQWebExecutor_removeRequestListener(&envView->executor, &envView->mainListener);

  NQCStrFree(envView->baseUrl);
}

static const struct NQWebExecutorOperations kWebEnvViewOps = {
  .init = executorInit,
  .release = executorRelease,
};

NQWebEnvView* NQWebEnvViewCreate(NQWebServer* server, const struct NQWebEnvViewParams* params)
{
  if (params->baseUrl == NULL)
    return NULL;

  return (NQWebEnvView*)NQWebServer_createExecutor(server, sizeof(struct NQWebEnvView), &kWebEnvViewOps, (void*)params);
}

void NQWebEnvViewDestroy(NQWebServer* server, NQWebEnvView* fileApi)
{
  NQWebServer_destroyExecutor(server, &fileApi->executor);
}
