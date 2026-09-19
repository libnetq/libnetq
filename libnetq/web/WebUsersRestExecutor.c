/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/web/WebUsersRestExecutor.h"

#include <libnetq/Malloc.h>
#include <libnetq/Time.h>
#include <libnetq/string/Sprintf.h>
#include <libnetq/string/Strtox.h>
#include <libnetq/string/StringPrint.h>
#include <libnetq/json/JSON.h>
#include <libnetq/web/JsonRpcResponse.h>
#include <libnetq/sql/UserDataStore.h>
#include <libnetq/crypto/JWT.h>
#include <libnetq/web/WebRequest.h>
#include <libnetq/web/WebResponse.h>
#include <libnetq/http/HttpHeader.h>
#include <libnetq/http/MediaType.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/Log.h>

#define BEARER_PREFIX "Bearer "

#define USE_USER_NAME     (1 << 0)
#define USE_USER_EMAIL    (1 << 1)
#define USE_USER_PASSWORD (1 << 2)

struct UserRequest {
  NQWebUsersRestListeners* listeners;
  NQStringPrint recvBuffer;
};

struct UserCredential {
  NQJSON* json;
  const char* name;
  const char* email;
  const char* password;
};

static bool jsonWriterHandler(void* userdata, const char* characters, size_t size)
{
  NQWebResponse* response = (NQWebResponse*)userdata;
  int n = NQWebResponse_write(response, characters, size);
  return n < 0 ? false : true;
}

static void UserCredential_init(struct UserCredential* thiz)
{
  thiz->json = NULL;
  thiz->name = NULL;
  thiz->email = NULL;
  thiz->password = NULL;
}

static void UserCredential_finalize(struct UserCredential* thiz)
{
  if (thiz->json != NULL) {
    NQJSON_release(thiz->json);
  }
}

static bool UserCredential_parse(struct UserCredential* thiz, const char* characters, size_t length, unsigned flags)
{
  NQJSON* json = NQJSON_parse2(characters, length);
  if (json == NULL) {
    NQ_LOGE("Invalid JSON");
    return false;
  }

  const char* name = NULL;
  if ((USE_USER_NAME & flags) && !NQJSON_objectGetString(json, "name", &name)) {
    NQ_LOGE("Missing required field: name");
    NQJSON_release(json);
    return false;
  }

  const char* email = NULL;
  if ((USE_USER_EMAIL & flags) && !NQJSON_objectGetString(json, "email", &email)) {
    NQ_LOGE("Missing required field: email");
    NQJSON_release(json);
    return false;
  }

  const char* password = NULL;
  if ((USE_USER_PASSWORD & flags) && !NQJSON_objectGetString(json, "password", &password)) {
    NQ_LOGE("Missing required field: password");
    NQJSON_release(json);
    return false;
  }

  if (thiz->json != NULL) {
    NQJSON_release(thiz->json);
  }

  thiz->json = json;
  thiz->name = name;
  thiz->email = email;
  thiz->password = password;

  return true;
}

struct JWTClaims {
  int64_t iss;
  int64_t exp;
  uint32_t sub;
};

static bool buildJWT(const struct JWTClaims* claims, const void* seckey, size_t sklen, char* buf, size_t len)
{
  int n;
  NQJWT* jwt = NQJWT_create(NQ_JWT_ALG_HS256);
  if (jwt == NULL) {
    NQ_LOGE("Failed to create JWT object");
    return false;
  }

  if (!NQJWT_claimSetInt64(jwt, NQ_JWT_CLM_ISS, claims->iss)) {
    NQ_LOGE("Failed to set JWT '" NQ_JWT_CLM_ISS "' claim");
    NQJWT_release(jwt);
    return false;
  }

  if (!NQJWT_claimSetInt64(jwt, NQ_JWT_CLM_EXP, claims->exp)) {
    NQ_LOGE("Failed to set JWT '" NQ_JWT_CLM_EXP "' claim");
    NQJWT_release(jwt);
    return false;
  }

  n = NQSnprintf(buf, len, "%u", claims->sub);
  if (n < 0 || len <= (size_t)n) {
    NQ_LOGE("Failed to format JWT '" NQ_JWT_CLM_SUB "' claim");
    NQJWT_release(jwt);
    return false;
  }

  if (!NQJWT_claimSetString(jwt, NQ_JWT_CLM_SUB, buf)) {
    NQ_LOGE("Failed to set JWT '" NQ_JWT_CLM_SUB "' claim");
    NQJWT_release(jwt);
    return false;
  }

  if (!NQJWT_sign(jwt, seckey, sklen)) {
    NQ_LOGE("Failed to sign JWT");
    NQJWT_release(jwt);
    return false;
  }

  n = NQJWT_token(jwt, buf, len);
  if (n < 0 || len <= (size_t)n) {
    NQ_LOGE("Failed to generate JWT token string");
    NQJWT_release(jwt);
    return false;
  }

  return true;
}

static bool parseJWT(const char* token, const void* seckey, size_t sklen, struct JWTClaims* claims)
{
  NQJWT* jwt = NQJWT_parse(token, seckey, sklen);
  if (jwt == NULL) {
    NQ_LOGE("Failed to parse JWT object");
    return false;
  }

  if (!NQJWT_claimGetInt64(jwt, NQ_JWT_CLM_ISS, &claims->iss)) {
    NQ_LOGE("Failed to get JWT '" NQ_JWT_CLM_ISS "' claim");
    NQJWT_release(jwt);
    return false;
  }

  if (!NQJWT_claimGetInt64(jwt, NQ_JWT_CLM_EXP, &claims->exp)) {
    NQ_LOGE("Failed to get JWT '" NQ_JWT_CLM_EXP "' claim");
    NQJWT_release(jwt);
    return false;
  }

  const char* sub;
  if (!NQJWT_claimGetString(jwt, NQ_JWT_CLM_SUB, &sub)) {
    NQ_LOGE("Failed to get JWT '" NQ_JWT_CLM_SUB "' claim");
    NQJWT_release(jwt);
    return false;
  }

  char* end;
  unsigned long num = NQSimpleStrtoul(sub, &end, 10);
  if (*end != '\0' || num > NQ_UINT32_MAX) {
    NQ_LOGE("Failed format of JWT '" NQ_JWT_CLM_SUB "' claim");
    NQJWT_release(jwt);
    return false;
  }

  claims->sub = (uint32_t)num;
  return true;
}

static bool getJWTClaims(NQWebRequest* request, struct JWTClaims* claims)
{
  const char* token = NQWebRequest_getHeader(request, NQHTTP_HEADER_AUTHORIZATION);
  if (token == NULL || !NQCStrStartsWith(token, BEARER_PREFIX)) {
    NQ_LOGE("Missing or malformed Authorization header");
    return false;
  }

  token += NQ_CSTR_LENGTH(BEARER_PREFIX);
  while (*token == ' ')
    token++;

  NQWebServer* server = NQWebRequest_server(request);
  if (!parseJWT(token, server->sessionSeckey, sizeof(server->sessionSeckey), claims)) {
    NQ_LOGE("Invalid or missing authentication token");
    return false;
  }

  if (claims->exp < NQGetTimeSec()) {
    NQ_LOGE("Authentication token expired");
    return false;
  }

  return true;
}

static int commonInit(NQWebRequest* request, void* data)
{
  struct NQWebUsersRestListeners* listeners = (NQWebUsersRestListeners*)data;
  struct UserRequest* userRequest = (struct UserRequest*)NQMalloc(sizeof(*userRequest));
  if (userRequest == NULL)
    return -NQ_ENOMEM;
  userRequest->listeners = listeners;
  NQStringPrint_init(&userRequest->recvBuffer);
  request->userdata = userRequest;
  return 0;
}

static size_t commonPostReceive(NQWebRequest* request, const char* data, size_t size)
{
  struct UserRequest* userRequest = (struct UserRequest*)request->userdata;
  return NQStringPrint_writeAll(&userRequest->recvBuffer, data, size) ? size : 0;
}

static int commonPostResponse(struct UserRequest* req, const char* email, NQWebResponse* response)
{
  NQWebServer* server = NQWebResponse_server(response);

  struct JWTClaims claims;
  if (!NQUserDataStoreUserId(req->listeners->database, email, &claims.sub)) {
    NQWebResponse_writeJsonRpcErrorParams(response, NQ_HTTP_INTERNAL_SERVER_ERROR, "Unable to get user id");
    return NQ_HTTP_INTERNAL_SERVER_ERROR;
  }

  claims.iss = NQGetTimeSec();
  claims.exp = claims.iss + 1 * NQ_HOURS_PER_DAY * NQ_MINS_PER_HOUR * NQ_SECS_PER_MIN; // 1d

  char buffer[256];
  if (!buildJWT(&claims, server->sessionSeckey, sizeof(server->sessionSeckey), buffer, sizeof(buffer))) {
    NQWebResponse_writeJsonRpcErrorParams(response, NQ_HTTP_INTERNAL_SERVER_ERROR, "Unable to create JWT");
    return NQ_HTTP_INTERNAL_SERVER_ERROR;
  }

  NQJSONWriter writer;
  NQJSONWriter_init(&writer, jsonWriterHandler, response);
  NQJSONWriter_writeObjectBegin(&writer);
  NQJSONWriter_writeKeyString(&writer, "token", buffer);
  NQJSONWriter_writeObjectEnd(&writer);
  NQJSONWriter_finalize(&writer);

  return NQ_HTTP_OK;
}

static int signupPostRequest(NQWebRequest* request, NQWebResponse* response)
{
  struct UserRequest* req = (struct UserRequest*)request->userdata;

  NQWebResponse_setHeader(response, NQHTTP_HEADER_CONTENT_TYPE, NQ_MEDIATYPE_APPLICATION_JSON);

  struct UserCredential credential;
  UserCredential_init(&credential);

  unsigned flags = USE_USER_NAME | USE_USER_EMAIL | USE_USER_PASSWORD;
  if (!UserCredential_parse(&credential, NQStringPrint_characters(&req->recvBuffer), NQStringPrint_length(&req->recvBuffer), flags)) {
    NQWebResponse_writeJsonRpcErrorParams(response, NQ_HTTP_BAD_REQUEST, "Invalid params");
    UserCredential_finalize(&credential);
    return NQ_HTTP_BAD_REQUEST;
  }

  if (NQStrlen(credential.password) < req->listeners->passwordMin) {
    NQWebResponse_writeJsonRpcErrorParams(response, NQ_HTTP_BAD_REQUEST, "'password'is shorter than the required length");
    UserCredential_finalize(&credential);
    return NQ_HTTP_BAD_REQUEST;
  }

  if (!NQUserDataStoreSignup(req->listeners->database, credential.email, credential.password)) {
    NQWebResponse_writeJsonRpcErrorParams(response, NQ_HTTP_INTERNAL_SERVER_ERROR, "Unable to register user");
    UserCredential_finalize(&credential);
    return NQ_HTTP_INTERNAL_SERVER_ERROR;
  }

  int ret = commonPostResponse(req, credential.email, response);
  UserCredential_finalize(&credential);
  return ret;
}

static int loginPostRequest(NQWebRequest* request, NQWebResponse* response)
{
  struct UserRequest* req = (struct UserRequest*)request->userdata;

  NQWebResponse_setHeader(response, NQHTTP_HEADER_CONTENT_TYPE, NQ_MEDIATYPE_APPLICATION_JSON);

  struct UserCredential credential;
  UserCredential_init(&credential);

  unsigned flags = USE_USER_EMAIL | USE_USER_PASSWORD;
  if (!UserCredential_parse(&credential, NQStringPrint_characters(&req->recvBuffer), NQStringPrint_length(&req->recvBuffer), flags)) {
    NQWebResponse_writeJsonRpcErrorParams(response, NQ_HTTP_BAD_REQUEST, "Invalid params");
    UserCredential_finalize(&credential);
    return NQ_HTTP_BAD_REQUEST;
  }

  if (!NQUserDataStoreLogin(req->listeners->database, credential.email, credential.password)) {
    NQWebResponse_writeJsonRpcErrorParams(response, NQ_HTTP_INTERNAL_SERVER_ERROR, "Incorrect username or password");
    UserCredential_finalize(&credential);
    return NQ_HTTP_INTERNAL_SERVER_ERROR;
  }

  int ret = commonPostResponse(req, credential.email, response);
  UserCredential_finalize(&credential);
  return ret;
}

static int updatePostRequest(NQWebRequest* request, NQWebResponse* response)
{
  struct UserRequest* req = (struct UserRequest*)request->userdata;

  NQWebResponse_setHeader(response, NQHTTP_HEADER_CONTENT_TYPE, NQ_MEDIATYPE_APPLICATION_JSON);

  struct JWTClaims claims;
  if (!getJWTClaims(request, &claims)) {
    NQWebResponse_writeJsonRpcErrorParams(response, NQ_HTTP_UNAUTHORIZED, "Invalid or missing authentication token");
    return NQ_HTTP_UNAUTHORIZED;
  }

  struct UserCredential credential;
  UserCredential_init(&credential);

  unsigned flags = USE_USER_EMAIL | USE_USER_PASSWORD;
  if (!UserCredential_parse(&credential, NQStringPrint_characters(&req->recvBuffer), NQStringPrint_length(&req->recvBuffer), flags)) {
    NQWebResponse_writeJsonRpcErrorParams(response, NQ_HTTP_BAD_REQUEST, "Invalid params");
    UserCredential_finalize(&credential);
    return NQ_HTTP_BAD_REQUEST;
  }

  uint32_t sub;
  if (!NQUserDataStoreUserId(req->listeners->database, credential.email, &sub)) {
    NQWebResponse_writeJsonRpcErrorParams(response, NQ_HTTP_INTERNAL_SERVER_ERROR, "Unable to get user id");
    UserCredential_finalize(&credential);
    return NQ_HTTP_INTERNAL_SERVER_ERROR;
  }

  if (claims.sub != sub) {
    NQWebResponse_writeJsonRpcErrorParams(response, NQ_HTTP_UNAUTHORIZED, "Invalid or missing authentication token");
    UserCredential_finalize(&credential);
    return NQ_HTTP_UNAUTHORIZED;
  }

  if (NQStrlen(credential.password) < req->listeners->passwordMin) {
    NQWebResponse_writeJsonRpcErrorParams(response, NQ_HTTP_BAD_REQUEST, "'password'is shorter than the required length");
    UserCredential_finalize(&credential);
    return NQ_HTTP_BAD_REQUEST;
  }

  if (!NQUserDataStoreUpdate(req->listeners->database, credential.email, credential.password)) {
    NQWebResponse_writeJsonRpcErrorParams(response, NQ_HTTP_INTERNAL_SERVER_ERROR, "Unable to update password");
    UserCredential_finalize(&credential);
    return NQ_HTTP_INTERNAL_SERVER_ERROR;
  }

  UserCredential_finalize(&credential);

  NQJSONWriter writer;
  NQJSONWriter_init(&writer, jsonWriterHandler, response);
  NQJSONWriter_writeBool(&writer, true);
  NQJSONWriter_finalize(&writer);

  return NQ_HTTP_OK;
}

static int deletePostRequest(NQWebRequest* request, NQWebResponse* response)
{
  struct UserRequest* req = (struct UserRequest*)request->userdata;

  NQWebResponse_setHeader(response, NQHTTP_HEADER_CONTENT_TYPE, NQ_MEDIATYPE_APPLICATION_JSON);

  struct JWTClaims claims;
  if (!getJWTClaims(request, &claims)) {
    NQWebResponse_writeJsonRpcErrorParams(response, NQ_HTTP_UNAUTHORIZED, "Invalid or missing authentication token");
    return NQ_HTTP_UNAUTHORIZED;
  }

  struct UserCredential credential;
  UserCredential_init(&credential);

  unsigned flags = USE_USER_EMAIL;
  if (!UserCredential_parse(&credential, NQStringPrint_characters(&req->recvBuffer), NQStringPrint_length(&req->recvBuffer), flags)) {
    NQWebResponse_writeJsonRpcErrorParams(response, NQ_HTTP_BAD_REQUEST, "Invalid params");
    UserCredential_finalize(&credential);
    return NQ_HTTP_BAD_REQUEST;
  }

  uint32_t sub;
  if (!NQUserDataStoreUserId(req->listeners->database, credential.email, &sub)) {
    NQWebResponse_writeJsonRpcErrorParams(response, NQ_HTTP_INTERNAL_SERVER_ERROR, "Unable to get user id");
    UserCredential_finalize(&credential);
    return NQ_HTTP_INTERNAL_SERVER_ERROR;
  }

  if (claims.sub != sub) {
    NQWebResponse_writeJsonRpcErrorParams(response, NQ_HTTP_BAD_REQUEST, "No permission to delete the user");
    UserCredential_finalize(&credential);
    return NQ_HTTP_BAD_REQUEST;
  }

  if (!NQUserDataStoreDelete(req->listeners->database, credential.email)) {
    NQWebResponse_writeJsonRpcErrorParams(response, NQ_HTTP_INTERNAL_SERVER_ERROR, "Unable to delete user");
    UserCredential_finalize(&credential);
    return NQ_HTTP_INTERNAL_SERVER_ERROR;
  }

  UserCredential_finalize(&credential);

  NQJSONWriter writer;
  NQJSONWriter_init(&writer, jsonWriterHandler, response);
  NQJSONWriter_writeBool(&writer, true);
  NQJSONWriter_finalize(&writer);

  return NQ_HTTP_OK;
}

static void commonPostRelease(NQWebRequest* request)
{
  struct UserRequest* req = (struct UserRequest*)request->userdata;
  NQStringPrint_finalize(&req->recvBuffer);
  NQFree(req);
}

static const NQWebRequestOperations kSignupOps = {
  .init    = commonInit,
  .receive = commonPostReceive,
  .handler = signupPostRequest,
  .release = commonPostRelease,
};

static const NQWebRequestOperations kLoginOps = {
  .init    = commonInit,
  .receive = commonPostReceive,
  .handler = loginPostRequest,
  .release = commonPostRelease,
};

static const NQWebRequestOperations kUpdateOps = {
  .init    = commonInit,
  .receive = commonPostReceive,
  .handler = updatePostRequest,
  .release = commonPostRelease,
};

static const NQWebRequestOperations kDeleteOps = {
  .init    = commonInit,
  .receive = commonPostReceive,
  .handler = deletePostRequest,
  .release = commonPostRelease,
};

int NQWebUsersRestListenersInit(NQWebExecutor* executor, NQWebUsersRestListeners* listeners, const NQWebUsersRestParams* params)
{
  listeners->database = NQSQLiteDatabase_open(params->databasePath, kNQSQLiteOpenCreateReadWrite);
  if (listeners->database == NULL) {
    return -NQ_EIO;
  }

  if (!NQUserDataStoreInit(listeners->database)) {
    NQSQLiteDatabase_release(listeners->database);
    return -NQ_EIO;
  }

  int ret = NQWebExecutor_addRequestListener(executor, &listeners->signupListener, &kSignupOps, listeners, NQ_HTTP_POST, params->signupUrl);
  if (ret) {
    NQUserDataStoreExit(listeners->database);
    NQSQLiteDatabase_release(listeners->database);
    return ret;
  }

  ret = NQWebExecutor_addRequestListener(executor, &listeners->loginListener, &kLoginOps, listeners, NQ_HTTP_POST, params->loginUrl);
  if (ret) {
    NQWebExecutor_removeRequestListener(executor, &listeners->signupListener);
    NQUserDataStoreExit(listeners->database);
    NQSQLiteDatabase_release(listeners->database);
    return ret;
  }

  ret = NQWebExecutor_addRequestListener(executor, &listeners->updateListener, &kUpdateOps, listeners, NQ_HTTP_POST, params->updateUrl);
  if (ret) {
    NQWebExecutor_removeRequestListener(executor, &listeners->loginListener);
    NQWebExecutor_removeRequestListener(executor, &listeners->signupListener);
    NQUserDataStoreExit(listeners->database);
    NQSQLiteDatabase_release(listeners->database);
    return ret;
  }

  ret = NQWebExecutor_addRequestListener(executor, &listeners->deleteListener, &kDeleteOps, listeners, NQ_HTTP_POST, params->deleteUrl);
  if (ret) {
    NQWebExecutor_removeRequestListener(executor, &listeners->updateListener);
    NQWebExecutor_removeRequestListener(executor, &listeners->loginListener);
    NQWebExecutor_removeRequestListener(executor, &listeners->signupListener);
    NQUserDataStoreExit(listeners->database);
    NQSQLiteDatabase_release(listeners->database);
    return ret;
  }

  listeners->passwordMin = params->passwordMin;

  return 0;
}

void NQWebUsersRestListenersFinalize(NQWebExecutor* executor, NQWebUsersRestListeners* listeners)
{
  NQWebExecutor_removeRequestListener(executor, &listeners->deleteListener);
  NQWebExecutor_removeRequestListener(executor, &listeners->updateListener);
  NQWebExecutor_removeRequestListener(executor, &listeners->loginListener);
  NQWebExecutor_removeRequestListener(executor, &listeners->signupListener);
  NQUserDataStoreExit(listeners->database);
  NQSQLiteDatabase_release(listeners->database);
}

static int executorInit(NQWebExecutor* executor, void* data)
{
  struct NQWebUsersRestParams* params = (struct NQWebUsersRestParams*)data;
  if (params->databasePath == NULL)
    return -NQ_EINVAL;
  struct NQWebUsersRestExecutor* usersApi = (struct NQWebUsersRestExecutor*)executor;
  return NQWebUsersRestListenersInit(&usersApi->executor, &usersApi->listeners, params);
}

static void executorRelease(NQWebExecutor* executor)
{
  struct NQWebUsersRestExecutor* usersApi = (struct NQWebUsersRestExecutor*)executor;
  NQWebUsersRestListenersFinalize(&usersApi->executor, &usersApi->listeners);
}

static const struct NQWebExecutorOperations kWebRestUsersOps = {
  .init = executorInit,
  .release = executorRelease,
};

NQWebUsersRestExecutor* NQWebUsersRestExecutorCreate(NQWebServer* server, const struct NQWebUsersRestParams* params)
{
  return (NQWebUsersRestExecutor*)NQWebServer_createExecutor(server, sizeof(struct NQWebUsersRestExecutor), &kWebRestUsersOps, (void*)params);
}

void NQWebUsersRestExecutorDestroy(NQWebServer* server, NQWebUsersRestExecutor* usersApi)
{
  NQWebServer_destroyExecutor(server, &usersApi->executor);
}
