/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/web/WebManifest.h"

#include <libnetq/json/JSON.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/List.h>
#include <libnetq/Log.h>
#include <libnetq/fs/Path.h>
#include <libnetq/Assert.h>
#include <libnetq/Malloc.h>
#include <libnetq/http/HttpHeader.h>
#include <libnetq/web/WebRequest.h>
#include <libnetq/web/WebResponse.h>

struct HeaderEntry {
  NQListHead list;
  const char* name;
  char value[1];
};

struct FileEntry {
  NQListHead list;
  struct NQWebRequestListener listener;
  NQPath* file;
  char* type;
  NQListHead headers;
};

struct NQWebManifestExecutor {
  NQWebExecutor executor;
  struct NQWebManifestListeners listeners;
};

static void fileHeadersFinalize(NQListHead* headers)
{
  while (!NQListHead_isEmpty(headers)) {
    struct HeaderEntry* entry = NQ_CONTAINER_OF(headers->next, struct HeaderEntry, list);
    NQListHead_remove(&entry->list);
    NQFree(entry);
  }
}

static int fileUrlMapInit(NQWebRequest* request, void* data)
{
  struct FileEntry* entry = (struct FileEntry*)data;
  request->userdata = entry;
  return 0;
}

static int fileUrlMapRequest(NQWebRequest* request, NQWebResponse* response)
{
  struct FileEntry* entry = (struct FileEntry*)NQWebRequest_userdata(request);

  NQUint8Array* arraybuffer = NQUint8Array_fromFile(NQPath_characters(entry->file));
  if (arraybuffer == NULL) {
    NQ_LOGE("Cannot load file '%s'", NQPath_characters(entry->file));
    return NQ_HTTP_INTERNAL_SERVER_ERROR;
  }

  NQListHead* hdrIter = entry->headers.next;
  while (hdrIter != &entry->headers) {
    struct HeaderEntry* hdrEntry = NQ_CONTAINER_OF(hdrIter, struct HeaderEntry, list);
    NQWebResponse_setHeader(response, hdrEntry->name, hdrEntry->value);
    hdrIter = hdrIter->next;
  }

  if (entry->type != NULL)
    NQWebResponse_setHeader(response, NQHTTP_HEADER_CONTENT_TYPE, entry->type);

  NQWebResponse_write(response, (char*)NQUint8Array_data(arraybuffer), NQUint8Array_size(arraybuffer));
  NQUint8Array_destroy(arraybuffer);
  return NQ_HTTP_OK;
}

static const NQWebRequestOperations kFileUrlMapOps = {
  .init = fileUrlMapInit,
  .handler = fileUrlMapRequest,
};

void NQWebManifestListenersFinalize(NQWebExecutor* executor, struct NQWebManifestListeners* listeners)
{
  while (!NQListHead_isEmpty(&listeners->entries)) {
    struct NQWebCatalogEntry* entry = NQ_CONTAINER_OF(listeners->entries.next, struct NQWebCatalogEntry, executorList);
    NQWebServer_removeCatalogEntry(executor->server, entry);
    NQListHead_remove(&entry->executorList);
    NQWebCatalogEntryDestroy(entry);
  }

  while (!NQListHead_isEmpty(&listeners->files)) {
    struct FileEntry* entry = NQ_CONTAINER_OF(listeners->files.next, struct FileEntry, list);
    NQWebExecutor_removeRequestListener(executor, &entry->listener);
    fileHeadersFinalize(&entry->headers);
    NQCStrFree(entry->type);
    NQPath_destroy(entry->file);
    NQListHead_remove(&entry->list);
    NQFree(entry);
  }
}

int NQWebManifestListenersInit(NQWebExecutor* executor, struct NQWebManifestListeners* listeners, const char* filename)
{
  NQJSON* rootJson = NQJSON_fromFile(filename);
  if (!rootJson)
    return -NQ_ENOENT;

  if (!NQJSON_isObject(rootJson)) {
    NQJSON_release(rootJson);
    return -NQ_EINVAL;
  }

  NQListHead_init(&listeners->files);
  NQListHead_init(&listeners->entries);

  const char* name = NULL;
  const char* baseUrl = NULL;

  NQJSON_objectGetString(rootJson, "name", &name);
  NQJSON_objectGetString(rootJson, "base", &baseUrl);

  NQJSON* filesJson = NQJSON_objectGet(rootJson, "files");
  if (NQJSON_isArray(filesJson)) {
    size_t size = NQJSON_arraySize(filesJson);
    for (size_t index = 0; index < size; index++) {
      NQJSON* iterJson = NQJSON_arrayGet(filesJson, index);

      const char* url;
      const char* file;
      const char* type;

      if (!NQJSON_objectGetString(iterJson, "url", &url) || !NQJSON_objectGetString(iterJson, "file", &file))
        continue;

      if (!NQJSON_objectGetString(iterJson, "type", &type))
        type = NQWebServer_getMimeType(executor->server, file);

      struct FileEntry* fileEntry = (struct FileEntry*)NQMalloc(sizeof(struct FileEntry));
      if (fileEntry == NULL) {
        NQWebManifestListenersFinalize(executor, listeners);
        NQJSON_release(rootJson);
        return -NQ_ENOMEM;
      }

      bool hasContentType = false;
      NQListHead_init(&fileEntry->headers);
      NQJSON* headersJson = NQJSON_objectGet(iterJson, "headers");
      if (NQJSON_isObject(headersJson)) {
        for (NQJSON_ObjectIter* hdrIter = NQJSON_objectIterFirst(headersJson); hdrIter != NULL; hdrIter = NQJSON_objectIterNext(headersJson, hdrIter)) {
          const char* name = NQJSON_objectIterKey(hdrIter);
          NQJSON* value = NQJSON_objectIterValue(hdrIter);
          if (!NQJSON_isString(value))
            continue;
          size_t nameLenz = NQStrlen(name) + 1;
          struct HeaderEntry* hdrEntry = NQMalloc(sizeof(*hdrEntry) + NQJSON_stringLength(value) + nameLenz);
          if (hdrEntry == NULL) {
            fileHeadersFinalize(&fileEntry->headers);
            NQWebManifestListenersFinalize(executor, listeners);
            NQJSON_release(rootJson);
            return -NQ_ENOMEM;
          }
          if (!NQStrcmp(NQHTTP_HEADER_CONTENT_TYPE, name))
            hasContentType = true;
          NQListHead_init(&hdrEntry->list);
          memcpy(hdrEntry->value, NQJSON_asString(value), NQJSON_stringLength(value) + 1);
          char* ptr = (char*)hdrEntry + sizeof(*hdrEntry) + NQJSON_stringLength(value);
          memcpy(ptr, name, nameLenz);
          hdrEntry->name = ptr;
          NQListHead_addBack(&fileEntry->headers, &hdrEntry->list);
        }
      }

      fileEntry->file = NQPath_join3(filename, NQ_PATH_PARENT_DIR_STR, file);
      if (fileEntry->file == NULL) {
        fileHeadersFinalize(&fileEntry->headers);
        NQFree(fileEntry);
        NQWebManifestListenersFinalize(executor, listeners);
        NQJSON_release(rootJson);
        return -NQ_ENOMEM;
      }

      fileEntry->type = NULL;
      if (!hasContentType) {
        fileEntry->type = NQCStrDuplicate(type);
        if (fileEntry->type == NULL) {
          fileHeadersFinalize(&fileEntry->headers);
          NQPath_destroy(fileEntry->file);
          NQFree(fileEntry);
          NQWebManifestListenersFinalize(executor, listeners);
          NQJSON_release(rootJson);
          return -NQ_ENOMEM;
        }
      }

      int ret = NQWebExecutor_addRequestListener(executor, &fileEntry->listener, &kFileUrlMapOps, fileEntry, NQ_HTTP_GET, "%s", url);
      if (ret != 0) {
        fileHeadersFinalize(&fileEntry->headers);
        NQCStrFree(fileEntry->type);
        NQPath_destroy(fileEntry->file);
        NQFree(fileEntry);
        NQWebManifestListenersFinalize(executor, listeners);
        NQJSON_release(rootJson);
        return ret;
      }

      NQListHead_addBack(&listeners->files, &fileEntry->list);
    }
  }

  NQJSON* entriesJson = NQJSON_objectGet(rootJson, "entries");
  if (NQJSON_isArray(entriesJson)) {
    size_t size = NQJSON_arraySize(entriesJson);
    for (size_t index = 0; index < size; index++) {
      NQJSON* entryJson = NQJSON_arrayGet(entriesJson, index);

      struct NQWebCatalogParams params = {
        .title = NULL,
        .mainUrl = NULL,
        .version = NULL,
        .description = NULL,

        .lightIconUrl = NULL,
        .darkIconUrl = NULL,

        .lightScreenshotUrl = NULL,
        .darkScreenshotUrl = NULL,
      };

      if (!NQJSON_objectGetString(entryJson, "main", &params.mainUrl))
        continue;

      NQJSON_objectGetString(entryJson, "title", &params.title);
      NQJSON_objectGetString(entryJson, "version", &params.version);
      NQJSON_objectGetString(entryJson, "description", &params.description);

      NQJSON* iconsJson = NQJSON_objectGet(entryJson, "icons");
      if (NQJSON_isArray(iconsJson)) {
        size_t size = NQJSON_arraySize(iconsJson);
        for (size_t index = 0; index < size; index++) {
          NQJSON* iterJson = NQJSON_arrayGet(iconsJson, index);
          const char* url;
          const char* colorScheme = "light";
          if (NQJSON_objectGetString(iterJson, "url", &url)) {
            (void)NQJSON_objectGetString(iterJson, "colorScheme", &colorScheme);
            if (!NQStrcmp(colorScheme, "light"))
              params.lightIconUrl = params.lightIconUrl ? params.lightIconUrl : url;
            else if (!NQStrcmp(colorScheme, "dark"))
              params.darkIconUrl = params.darkIconUrl ? params.darkIconUrl : url;
          }
        }
      }

      NQJSON* screenshotsJson = NQJSON_objectGet(entryJson, "screenshots");
      if (NQJSON_isArray(screenshotsJson)) {
        size_t size = NQJSON_arraySize(screenshotsJson);
        for (size_t index = 0; index < size; index++) {
          NQJSON* iterJson = NQJSON_arrayGet(screenshotsJson, index);
          const char* url;
          const char* colorScheme = "light";
          if (NQJSON_objectGetString(iterJson, "url", &url)) {
            (void)NQJSON_objectGetString(iterJson, "colorScheme", &colorScheme);
            if (!NQStrcmp(colorScheme, "light"))
              params.lightScreenshotUrl = params.lightScreenshotUrl ? params.lightScreenshotUrl : url;
            else if (!NQStrcmp(colorScheme, "dark"))
              params.darkScreenshotUrl = params.darkScreenshotUrl ? params.darkScreenshotUrl : url;
          }
        }
      }

      struct NQWebCatalogEntry* entry = NQWebCatalogEntryCreate(&params);
      if (entry) {
        NQWebServer_addCatalogEntry(executor->server, entry);
        NQListHead_addBack(&listeners->entries, &entry->executorList);
      }
    }
  }

  NQJSON_release(rootJson);
  return 0;
}

static int executorInit(NQWebExecutor* exec, void* data)
{
  const char* filename = (const char*)data;
  struct NQWebManifestExecutor* manifest = NQ_CONTAINER_OF(exec, struct NQWebManifestExecutor, executor);
  return NQWebManifestListenersInit(&manifest->executor, &manifest->listeners, filename);
}

static void executorRelease(NQWebExecutor* exec)
{
  struct NQWebManifestExecutor* manifest = NQ_CONTAINER_OF(exec, struct NQWebManifestExecutor, executor);
  NQWebManifestListenersFinalize(&manifest->executor, &manifest->listeners);
}

static const struct NQWebExecutorOperations kNQWebManifestOps = {
  .init = executorInit,
  .release = executorRelease,
};

NQWebManifestExecutor* NQWebManifestExecutorCreate(NQWebServer* server, const char* filename)
{
  return (NQWebManifestExecutor*)NQWebServer_createExecutor(server, sizeof(struct NQWebManifestExecutor), &kNQWebManifestOps, (void*)filename);
}

void NQWebManifestExecutorDestroy(NQWebServer* server, NQWebManifestExecutor* manifest)
{
  NQWebServer_destroyExecutor(server, &manifest->executor);
}
