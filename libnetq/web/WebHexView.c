/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/web/WebHexView.h"

#include <libnetq/string/StringPrint.h>
#include <libnetq/string/String.h>
#include <libnetq/http/HttpHeader.h>
#include <libnetq/http/HttpMultiPartParser.h>
#include <libnetq/ByteBuffer.h>
#include <libnetq/json/JSONWriter.h>
#include <libnetq/http/MediaType.h>
#include <libnetq/Env.h>
#include <libnetq/Event.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/URL.h>
#include <libnetq/Assert.h>
#include <libnetq/Malloc.h>
#include <libnetq/web/JsonRpcTypes.h>
#include <libnetq/web/WebRequest.h>
#include <libnetq/web/WebResponse.h>
#include <libnetq/HexViewer.h>

enum FieldType {
  kUnknownField,
  kContentDispositionField,
  kContentTypeField,
};

enum ContentDataState {
  kContentInit,
  kContentProcess,
  kContentComplite,
};

struct NQWebHexView {
  NQWebExecutor executor;
  struct NQWebRequestListener mainListener;
  struct NQWebRequestListener postListener;
  char* baseUrl;
};

#define kContentTypeMax (32)

struct PostRequest {
  NQWebHexView* hexView;
  NQHTTPMultiPartParser parser;
  uint8_t lastFieldType;
  char lastContentType[kContentTypeMax];

  uint8_t inFileState;

  NQStringPrint filename;
  char contentType[kContentTypeMax];
  bool hasContentData;
  NQByteBuffer contentData;
};

static int mainInit(NQWebRequest* request, void* data)
{
  struct NQWebHexView* hexView = (NQWebHexView*)data;
  request->userdata = hexView;
  return 0;
}

static int requestPrintf(struct NQWebHexView* hexView, NQWebRequest* request, NQWebResponse* response, const struct PostRequest* post)
{
  NQWebResponse_setHeader(response, NQHTTP_HEADER_CONTENT_TYPE, NQ_MEDIATYPE_TEXT_HTML);

  NQWebResponse_printf(response, "<!DOCTYPE html>");
  NQWebResponse_printf(response, "<html>");
  NQWebResponse_printf(response, "<head>");
  NQWebResponse_printf(response, "<meta charset=\"UTF-8\">");
  NQWebResponse_printf(response, "<title>Hex Viewer</title>");
  NQWebResponse_printf(response, "</head>");
  NQWebResponse_printf(response, "<body>");
  NQWebResponse_printf(response, "<form action=\"%s\" method=\"POST\" enctype=\"multipart/form-data\">", hexView->baseUrl);
  NQWebResponse_printf(response, "<input type=\"file\" name=\"file\" style=\"color:transparent\" onchange=\"this.form.submit()\">");

  NQWebResponse_printf(response, "<form>");
  NQWebResponse_printf(response, "<hr>");

  if (post != NULL) {
    if (NQStringPrint_length(&post->filename))
      NQWebResponse_printf(response, "<div>Filename: %s</div>", NQStringPrint_characters(&post->filename));
    NQWebResponse_printf(response, "<div>Length: %llu</div>", (unsigned long long)NQByteBuffer_size(&post->contentData));
    if (*post->contentType)
      NQWebResponse_printf(response, "<div>ContentType: %s</div>", post->contentType);

    NQWebResponse_printf(response, "<pre>");

    NQHexViewer* hexViewer = NQHexViewer_create(NQ_HEXVIEWER_UPPERCASTE);
    if (hexViewer != NULL) {
      NQHexViewer_setOffset(hexViewer, 0);
      NQHexViewer_addData(hexViewer, NQByteBuffer_data(&post->contentData), NQByteBuffer_size(&post->contentData));
      NQWebResponse_write(response, NQHexViewer_characters(hexViewer), NQHexViewer_length(hexViewer));
      NQHexViewer_destroy(hexViewer);
    }

    NQWebResponse_printf(response, "</pre>");
  }

  NQWebResponse_printf(response, "</body>");
  NQWebResponse_printf(response, "</html>");

  return NQ_HTTP_OK;
}

static int mainHandler(NQWebRequest* request, NQWebResponse* response)
{
  struct NQWebHexView* hexView = (struct NQWebHexView*)request->userdata;
  requestPrintf(hexView, request, response, NULL);
  return NQ_HTTP_OK;
}

static const NQWebRequestOperations kMainRequestOps = {
  .init = mainInit,
  .handler = mainHandler,
};

static bool WebRequest_onMultiPartParser(void* userdata, NQHTTPMultiPartType type, const char* data, size_t size)
{
  struct PostRequest* ctx = (struct PostRequest*)userdata;
  const char* httpHeaderContentType = NQHTTP_HEADER_CONTENT_TYPE;
  const char* httpHeaderContentDisposition = NQHTTP_HEADER_CONTENT_DISPOSITION;

  switch (type) {
  case kNQHTTPMultiPartBegin:
    return true;

  case kNQHTTPMultiPartBoundary:
    if (ctx->inFileState == kContentProcess) {
      ctx->inFileState = kContentComplite;
      ctx->hasContentData = true;
    }
    return true;

  case kNQHTTPMultiPartFieldName:
    if (NQStrlen(httpHeaderContentDisposition) == size && memcmp(httpHeaderContentDisposition, data, size) == 0)
      ctx->lastFieldType = kContentDispositionField;
    else if (NQStrlen(httpHeaderContentType) == size && memcmp(httpHeaderContentType, data, size) == 0)
      ctx->lastFieldType = kContentTypeField;
    else
      ctx->lastFieldType = kUnknownField;
    return true;

  case kNQHTTPMultiPartFieldValue:
    if (ctx->lastFieldType == kContentDispositionField) {
      NQHttpFormData formData;
      if (NQHttpFormDataParse(data, size, &formData)) {
        if (ctx->inFileState == kContentInit) {
          if (NQStringRangeIsEqual(&formData.name, "file")) {
            NQStringPrint_write(&ctx->filename, formData.filename.characters, formData.filename.length);
            memcpy(ctx->contentType, ctx->lastContentType, kContentTypeMax);
            ctx->inFileState = kContentProcess;
          }
        }
      }
    }
    else if (ctx->lastFieldType == kContentTypeField) {
      char* str = NULL;
      if (ctx->inFileState == kContentInit) {
        str = ctx->lastContentType;
      }
      else if (ctx->inFileState == kContentProcess) {
        str = ctx->contentType;
      }
      if (str != NULL) {
        memset(str, 0, kContentTypeMax);
        if (size >= kContentTypeMax) {
          return false;
        }
        memcpy(str, data, size);
      }
    }
    return true;

  case kNQHTTPMultiPartBodyPart:
    if (ctx->inFileState == kContentProcess) {
      NQByteBuffer_append(&ctx->contentData, (const uint8_t*)data, size);
    }
    return true;

  case kNQHTTPMultiPartFinish:
    return true;
  }

  return false;
}

static int postInit(NQWebRequest* request, void* data)
{
  struct NQWebHexView* hexView = (NQWebHexView*)data;

  const char* contentType = NQWebRequest_getHeader(request, NQHTTP_HEADER_CONTENT_TYPE);
  if (contentType == NULL)
    return -NQ_EINVAL;

  const char* boundary = NQHttpGetContentBoundary(contentType);
  if (boundary == NULL)
    return -NQ_EINVAL;

  struct PostRequest* ctx = (struct PostRequest*)NQZalloc(sizeof(struct PostRequest));
  if (ctx == NULL) {
    return -NQ_ENOMEM;
  }

  ctx->hexView = hexView;
  NQHTTPMultiPartParser_init(&ctx->parser, boundary, &WebRequest_onMultiPartParser, ctx);
  ctx->lastFieldType = kUnknownField;
  ctx->inFileState = kContentInit;

  NQStringPrint_init(&ctx->filename);

  memset(ctx->lastContentType, 0, sizeof(ctx->lastContentType));
  memset(ctx->contentType, 0, sizeof(ctx->contentType));

  ctx->hasContentData = false;
  NQByteBuffer_init(&ctx->contentData);

  request->userdata = ctx;
  return 0;
}

static size_t postReceive(NQWebRequest* request, const char* data, size_t size)
{
  struct PostRequest* ctx = (struct PostRequest*)request->userdata;

  NQHTTPMultiPartParser_append(&ctx->parser, data, size);
  return size;
}

static int postHandler(NQWebRequest* request, NQWebResponse* response)
{
  struct PostRequest* ctx = (struct PostRequest*)NQWebRequest_userdata(request);

  if (!NQHTTPMultiPartParser_finish(&ctx->parser) || !ctx->hasContentData)
    return NQ_HTTP_BAD_REQUEST;

  requestPrintf(ctx->hexView, request, response, ctx);
  return NQ_HTTP_OK;
}

static void postRelease(NQWebRequest* request)
{
  struct PostRequest* ctx = (struct PostRequest*)NQWebRequest_userdata(request);

  NQHTTPMultiPartParser_finalize(&ctx->parser);
  NQStringPrint_finalize(&ctx->filename);
  NQByteBuffer_finalize(&ctx->contentData);

  NQFree(ctx);
}

static const NQWebRequestOperations kPostRequestOps = {
  .init = postInit,
  .receive = postReceive,
  .handler = postHandler,
  .release = postRelease,
};

static int executorInit(NQWebExecutor* executor, void* data)
{
  int ret;

  struct NQWebHexViewParams* params = (struct NQWebHexViewParams*)data;
  struct NQWebHexView* hexView = NQ_CONTAINER_OF(executor, struct NQWebHexView, executor);

  hexView->baseUrl = NQCStrDuplicate(params->baseUrl);
  if (hexView->baseUrl == NULL) {
    return -NQ_ENOMEM;
  }

  ret = NQWebExecutor_addRequestListener(&hexView->executor, &hexView->mainListener, &kMainRequestOps, hexView, NQ_HTTP_GET, "%s", params->baseUrl);
  if (ret) {
    NQCStrFree(hexView->baseUrl);
    return ret;
  }

  ret = NQWebExecutor_addRequestListener(&hexView->executor, &hexView->postListener, &kPostRequestOps, hexView, NQ_HTTP_POST, "%s", params->baseUrl);
  if (ret) {
    NQWebExecutor_removeRequestListener(&hexView->executor, &hexView->mainListener);
    NQCStrFree(hexView->baseUrl);
    return ret;
  }

  return 0;
}

static void executorRelease(NQWebExecutor* executor)
{
  struct NQWebHexView* hexView = NQ_CONTAINER_OF(executor, struct NQWebHexView, executor);

  NQWebExecutor_removeRequestListener(&hexView->executor, &hexView->postListener);
  NQWebExecutor_removeRequestListener(&hexView->executor, &hexView->mainListener);

  NQCStrFree(hexView->baseUrl);
}

static const struct NQWebExecutorOperations kWebHexViewOps = {
  .init = executorInit,
  .release = executorRelease,
};

NQWebHexView* NQWebHexViewCreate(NQWebServer* server, const struct NQWebHexViewParams* params)
{
  if (params->baseUrl == NULL)
    return NULL;

  return (NQWebHexView*)NQWebServer_createExecutor(server, sizeof(struct NQWebHexView), &kWebHexViewOps, (void*)params);
}

void NQWebHexViewDestroy(NQWebServer* server, NQWebHexView* fileApi)
{
  NQWebServer_destroyExecutor(server, &fileApi->executor);
}
