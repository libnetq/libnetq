/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/web/WebFileView.h"

#include <libnetq/ErrorCode.h>
#include <libnetq/string/Sprintf.h>
#include <libnetq/string/StringUtil.h>
#include <libnetq/http/HttpHeader.h>
#include <libnetq/http/HttpStatus.h>
#include <libnetq/http/MediaType.h>
#include <libnetq/fs/FileHandle.h>
#include <libnetq/fs/Stat.h>
#include <libnetq/fs/Dir.h>
#include <libnetq/fs/Path.h>
#include <libnetq/Log.h>
#include <libnetq/Malloc.h>
#include <libnetq/Assert.h>
#include <libnetq/web/WebRequest.h>
#include <libnetq/web/WebResponse.h>

#define SIZE_EMPTY_STR "      -"
#define DATETIME_EMPTY_STR "ACCESS_DENIED      "

#define KB NQ_UINT64_C(1024)
#define MB (KB * 1024)
#define GB (MB * 1024)
#define TB (GB * 1024)

struct NQWebFileView {
  NQWebExecutor executor;
  struct NQWebRequestListener mainListener;
  struct NQWebRequestListener otherListener;
  char* baseDir;
  char* baseUrl;
};

static void printFileSize(char* buf, size_t len, uint64_t fileSize)
{
  char* ptr = buf + len - 1;
  if (ptr >= buf)
    *ptr-- = '\0';
  if (ptr >= buf) {
    if (fileSize < KB)
      /* */;
    else if (fileSize < MB) {
      fileSize /= KB;
      *ptr-- = 'K';
    }
    else if (fileSize < GB) {
      fileSize /= MB;
      *ptr-- = 'M';
    }
    else if (fileSize < TB) {
      fileSize /= MB;
      *ptr-- = 'G';
    }
    else {
      fileSize /= TB;
      *ptr-- = 'T';
    }
  }
  while (ptr >= buf) {
    if (fileSize == 0)
      *ptr-- = ' ';
    else {
      *ptr-- = '0' + (fileSize % 10);
      fileSize /= 10;
    }
  }
}

static const char* getRelativePath(const char* baseUrl, const char* url)
{
  size_t baseUrlLength = NQStrlen(baseUrl);
  size_t urlLength = NQStrlen(url);

  if (urlLength < baseUrlLength || memcmp(url, baseUrl, baseUrlLength) != 0)
    return NULL;

  return url + baseUrlLength;
}

static int onGetRequest(NQWebRequest* request, NQWebResponse* response)
{
  struct NQWebFileView* fileApi = (struct NQWebFileView*)request->userdata;

  NQWebServer* server = NQWebRequest_server(request);

  const char* url = NQWebRequest_url(request);
  const char* relativePath = getRelativePath(fileApi->baseUrl, url);

  if (relativePath == NULL || strstr(url, "//") != NULL || strstr(url, "/../") != NULL) {
    NQWebResponse_setHeader(response, NQHTTP_HEADER_LOCATION, fileApi->baseUrl);
    return NQ_HTTP_MOVED_PERMANENTLY;
  }

  NQStringPrint pathBuf;
  NQStringPrint_init(&pathBuf);
  if (NQStringPrint_printf(&pathBuf, "%s%s", fileApi->baseDir, relativePath) < 0) {
    NQStringPrint_finalize(&pathBuf);
    return NQ_HTTP_INTERNAL_SERVER_ERROR;
  }

  char* path = NQStringPrint_characters(&pathBuf);
  size_t pathLen = NQStringPrint_length(&pathBuf);

  NQStat stat;
  if (!NQIsPathSeparator(path[pathLen - 1])) {
    int err = NQGetStat(path, &stat);
    if (err) {
      NQ_LOGE("NQGetStat error %i", err);
      NQStringPrint_finalize(&pathBuf);
      return NQ_HTTP_INTERNAL_SERVER_ERROR;
    }

    if (NQStat_isFile(&stat)) {
      const char* contentType = NQWebServer_getMimeType(server, path);
      NQFileHandle handle;
      int ret = NQFileOpen(path, NQ_FOPEN_READ, &handle);
      NQStringPrint_finalize(&pathBuf);
      if (ret != 0) {
        return NQ_HTTP_INTERNAL_SERVER_ERROR;
      }

      NQWebResponse_setHeader(response, NQHTTP_HEADER_CONTENT_TYPE, contentType);

      uint8_t buffer[1024];
      for (;;) {
        int len = NQFileRead(handle, buffer, sizeof(buffer));
        if (len < 0) {
          NQFileClose(handle);
          return NQ_HTTP_INTERNAL_SERVER_ERROR;
        }
        if (len == 0)
          break;
        NQWebResponse_write(response, (char*)buffer, (size_t)len);
      }

      NQFileClose(handle);
      return NQ_HTTP_OK;
    }

    NQStringPrint_finalize(&pathBuf);

    if (!NQStat_isDirectory(&stat))
      return NQ_HTTP_NOT_FOUND;

    NQStringPrint_init(&pathBuf);
    if (NQStringPrint_printf(&pathBuf, "%s/", url) < 0) {
      NQStringPrint_finalize(&pathBuf);
      return NQ_HTTP_INTERNAL_SERVER_ERROR;
    }

    const char* location = NQPathBuilder_characters(&pathBuf);
    NQWebResponse_setHeader(response, NQHTTP_HEADER_LOCATION, location);
    NQStringPrint_finalize(&pathBuf);
    return NQ_HTTP_MOVED_PERMANENTLY;
  }

  NQDir* dir =  NQDir_open(path);

  if (dir == NULL) {
    NQStringPrint_finalize(&pathBuf);
    return NQ_HTTP_NOT_FOUND;
  }

  NQWebResponse_setHeader(response, NQHTTP_HEADER_CONTENT_TYPE, NQ_MEDIATYPE_TEXT_HTML);
  NQWebResponse_printf(response, "<!DOCTYPE html>");
  NQWebResponse_printf(response, "<html>");
  NQWebResponse_printf(response, "<head>");
  NQWebResponse_printf(response, "<title>Index of /%s</title>", relativePath);
  NQWebResponse_printf(response, "</head>");
  NQWebResponse_printf(response, "<body>");
  NQWebResponse_printf(response, "<h1>Index of /%s</h1>", relativePath);
  NQWebResponse_printf(response, "<hr><pre>");

  if (*relativePath != '\0') {
    NQWebResponse_printf(response, "<a href=\"../\">..</a>\n");
  }

  static const char spaceStr[] = "                                                                               ";

  char sizeBuf[NQ_ARRAY_LENGTH(SIZE_EMPTY_STR)];
  char datetimeBuf[NQ_ARRAY_LENGTH(DATETIME_EMPTY_STR)];

  while (NQDir_next(dir)) {
    const char* name = NQDir_name(dir);
    if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
      continue;
    size_t nlen = NQStrlen(name);

    const char* delimeter = "";
    const char* sizeStr = SIZE_EMPTY_STR;
    const char* datetimeStr = DATETIME_EMPTY_STR;

    if (NQStringPrint_writeAll(&pathBuf, name, nlen)) {
      if (NQGetStat(NQStringPrint_characters(&pathBuf), &stat) == 0) {
        printFileSize(sizeBuf, sizeof(sizeBuf), NQStat_size(&stat));
        int sz = NQTimeMsFormat(NQStat_modificationTimeMs(&stat), NQ_DT_RFC3339, datetimeBuf, sizeof(datetimeBuf));
        NQ_ASSERT(sz == NQ_CSTR_LENGTH(DATETIME_EMPTY_STR));
        sizeStr = sizeBuf;
        datetimeStr = datetimeBuf;
      }
      NQStringPrint_resize(&pathBuf, pathLen);
    }

    size_t spaceLen = sizeof(spaceStr) - nlen - sizeof(sizeBuf) - 1 - sizeof(datetimeBuf) - 1;
    if (NQDir_isDirectory(dir)) {
      delimeter = "/";
      spaceLen--;
    }

    NQWebResponse_printf(response, "<a href=\"%s%s\">%s%s</a>%*.s%s%s\n", name, delimeter, name, delimeter, (int)spaceLen, spaceStr, datetimeStr, sizeStr);
  }
  NQDir_close(dir);

  NQWebResponse_printf(response, "</pre><hr>");
  NQWebResponse_printf(response, "</body>");
  NQWebResponse_printf(response, "</html>");

  NQStringPrint_finalize(&pathBuf);
  return NQ_HTTP_OK;
}

static int onFsInit(NQWebRequest* request, void* data)
{
  struct NQWebFileView* fileApi = NQ_CONTAINER_OF(data, struct NQWebFileView, executor);
  request->userdata = fileApi;
  return 0;
}

static const NQWebRequestOperations kFsOps = {
  .init = onFsInit,
  .handler = onGetRequest,
};

static int restApiInit(NQWebExecutor* executor, void* data)
{
  int ret;
  struct NQWebFileViewParams* params = (struct NQWebFileViewParams*)data;
  struct NQWebFileView* fileApi = NQ_CONTAINER_OF(executor, struct NQWebFileView, executor);

  NQPathBuilder pathBld;
  NQPathBuilder_init(&pathBld);
  if (!NQPathBuilder_resolve2(&pathBld, NQWebServer_workDir(executor->server), params->baseDir)) {
    NQPathBuilder_finalize(&pathBld);
    return -NQ_ENOMEM;
  }

  fileApi->baseDir = NQCStrFormat("%s" NQ_PATH_SEPARATOR_STR, NQPathBuilder_characters(&pathBld));
  NQPathBuilder_finalize(&pathBld);

  if (fileApi->baseDir == NULL) {
    return -NQ_ENOMEM;
  }

  fileApi->baseUrl = NQCStrFormat("%s" NQ_PATH_SEPARATOR_STR, params->baseUrl);
  if (fileApi->baseUrl == NULL) {
    NQCStrFree(fileApi->baseDir);
    return -NQ_ENOMEM;
  }

  ret = NQWebExecutor_addRequestListener(&fileApi->executor, &fileApi->mainListener, &kFsOps, fileApi, NQ_HTTP_GET, "%s", params->baseUrl);
  if (ret) {
    NQCStrFree(fileApi->baseDir);
    NQCStrFree(fileApi->baseUrl);
    return ret;
  }

  ret = NQWebExecutor_addRequestListener(&fileApi->executor, &fileApi->otherListener, &kFsOps, fileApi, NQ_HTTP_GET, "%s" NQ_PATH_SEPARATOR_STR "*", params->baseUrl);
  if (ret) {
    NQWebExecutor_removeRequestListener(&fileApi->executor, &fileApi->mainListener);
    NQCStrFree(fileApi->baseDir);
    NQCStrFree(fileApi->baseUrl);
    return ret;
  }

  return 0;
}

static void restApiRelease(NQWebExecutor* executor)
{
  struct NQWebFileView* fileApi = NQ_CONTAINER_OF(executor, struct NQWebFileView, executor);

  NQWebExecutor_removeRequestListener(&fileApi->executor, &fileApi->otherListener);
  NQWebExecutor_removeRequestListener(&fileApi->executor, &fileApi->mainListener);

  NQCStrFree(fileApi->baseDir);
  NQCStrFree(fileApi->baseUrl);
}

static const struct NQWebExecutorOperations kWebFileViewOps = {
  .init = restApiInit,
  .release = restApiRelease,
};

NQWebFileView* NQWebFileViewCreate(NQWebServer* server, const struct NQWebFileViewParams* params)
{
  if (params->baseUrl == NULL || params->baseDir == NULL)
    return NULL;

  return (NQWebFileView*)NQWebServer_createExecutor(server, sizeof(struct NQWebFileView), &kWebFileViewOps, (void*)params);
}

void NQWebFileViewDestroy(NQWebServer* server, NQWebFileView* fileApi)
{
  NQWebServer_destroyExecutor(server, &fileApi->executor);
}
