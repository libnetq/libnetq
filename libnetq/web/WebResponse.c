/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/web/WebResponse.h"

#include <libnetq/string/String.h>
#include <libnetq/http/HttpHeader.h>
#include <libnetq/Malloc.h>
#include <libnetq/web/WebRequest.h>
#include <libnetq/web/WebServer.h>

static int platformWrite(NQWebWriter* writer, const void* data, size_t size)
{
  NQWebResponse* thiz = NQ_CONTAINER_OF(writer, struct NQWebResponse, lastWriter);
  return thiz->operations->write(thiz, data, size);
}

static int platformFlush(NQWebWriter* writer)
{
  NQWebResponse* thiz = NQ_CONTAINER_OF(writer, struct NQWebResponse, lastWriter);
  return thiz->operations->flush(thiz);
}

static const NQWebWriterOperations kPlatformWriterOperations =
{
  .init = NULL,
  .write = platformWrite,
  .flush = platformFlush,
  .release = NULL,
};

void NQWebResponse_init(NQWebResponse* thiz, const struct NQWebResponseOperations* operations, NQWebRequest* request)
{
  thiz->operations = operations;
  thiz->server = NQWebRequest_server(request);
  thiz->request = request;

  NQStringPrint_init(&thiz->printfBuffer);

  thiz->firstWriter = &thiz->lastWriter;
  thiz->lastWriter.operations = &kPlatformWriterOperations;
  thiz->lastWriter.next = NULL;
  thiz->lastWriter.request = request;
  thiz->lastWriter.userdata = NULL;
}

void NQWebResponse_finalize(NQWebResponse* thiz)
{
  NQWebWriter* iter = thiz->firstWriter;
  while (iter) {
    NQWebWriter* next = iter->next;
    if (iter->operations->release)
      iter->operations->release(iter);
    if (iter != &thiz->lastWriter)
      NQFree(iter);
    iter = next;
  }

  NQStringPrint_finalize(&thiz->printfBuffer);
}

bool NQWebResponse_setHeader(NQWebResponse* thiz, const char* header, const char* value)
{
  if (!NQStrcmp(NQHTTP_HEADER_CONTENT_TYPE, header)) {
    NQWebWriter* writerChain = NQWebServer_createWriterChain(thiz->server, value, thiz->request);
    if (writerChain) {
      NQWebWriter* lastWriter = writerChain;
      while (lastWriter->next)
        lastWriter = lastWriter->next;
      lastWriter->next = thiz->firstWriter;
      thiz->firstWriter = writerChain;
    }
  }

  return thiz->operations->setHeader(thiz, header, value);
}

int NQWebResponse_printf(NQWebResponse* thiz, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int ret = NQWebResponse_vprintf(thiz, fmt, args);
  va_end(args);
  return ret;
}

int NQWebResponse_vprintf(NQWebResponse* thiz, const char* fmt, va_list args)
{
  int result = NQStringPrint_vprintf(&thiz->printfBuffer, fmt, args);
  if (result > 0) {
    const char* characters = NQStringPrint_characters(&thiz->printfBuffer);
    size_t length = NQStringPrint_length(&thiz->printfBuffer);
    result = NQWebWriter_write(thiz->firstWriter, characters, length);
    NQStringPrint_reset(&thiz->printfBuffer);
  }
  return result;
}

int NQWebResponse_write(NQWebResponse* thiz, const void* data, size_t size)
{
  return NQWebWriter_write(thiz->firstWriter, data, size);
}

int NQWebResponse_flush(NQWebResponse* thiz)
{
  return NQWebWriter_flush(thiz->firstWriter);
}
