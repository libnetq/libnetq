/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/web/JsonRpcResponse.h"

#include <libnetq/web/JsonRpcTypes.h>
#include <libnetq/string/StringPrint.h>
#include <libnetq/json/JSONWriter.h>

static bool writeResponse(void* userdata, const char* characters, size_t length)
{
  NQWebResponse* response = (NQWebResponse*)userdata;
  int n = NQWebResponse_write(response, characters, length);
  return n < 0 ? false : true;
}

static bool NQJSONWriter_writeJsonRpcErrorParams(NQJSONWriter* writer, int code, const char* message)
{
  if (!NQJSONWriter_writeObjectBegin(writer))
    return false;

  if (code != 0) {
    if (!NQJSONWriter_writeKeyUint32(writer, NQ_JSONRPC_NAME_CODE, code))
      return false;
  }

  if (message != NULL) {
    if (!NQJSONWriter_writeKeyString(writer, NQ_JSONRPC_NAME_MESSAGE, message))
      return false;
  }

  if (!NQJSONWriter_writeObjectEnd(writer))
    return false;

  return true;
}

bool NQWebResponse_writeJsonRpcErrorParams(NQWebResponse* response, int code, const char* message)
{
  NQJSONWriter writer;
  NQJSONWriter_init(&writer, &writeResponse, response);
  bool success = NQJSONWriter_writeJsonRpcErrorParams(&writer, code, message);
  NQJSONWriter_finalize(&writer);
  return success;
}

bool NQWebResponse_writeJsonRpcError(NQWebResponse* response, uint32_t id, int code, const char* message)
{
  NQJSONWriter writer;
  NQJSONWriter_init(&writer, &writeResponse, response);

  NQJSONWriter_writeObjectBegin(&writer);

  NQJSONWriter_writeKey(&writer, NQ_JSONRPC_NAME_ERROR);
  NQJSONWriter_writeJsonRpcErrorParams(&writer, code, message);

  if (id != 0)
    NQJSONWriter_writeKeyUint32(&writer, NQ_JSONRPC_NAME_ID, id);
  else
    NQJSONWriter_writeKeyNull(&writer, NQ_JSONRPC_NAME_ID);

  NQJSONWriter_writeObjectEnd(&writer);
  bool suceess = !writer.hasError;
  NQJSONWriter_finalize(&writer);
  return suceess;
}
