/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_WEB_WEBUSERSRESTEXECUTOR_H
#define _LIBNETQ_WEB_WEBUSERSRESTEXECUTOR_H

#include <libnetq/web/WebServer.h>
#include <libnetq/sql/SQLite.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQWebUsersRestParams NQWebUsersRestParams;
typedef struct NQWebUsersRestListeners NQWebUsersRestListeners;
typedef struct NQWebUsersRestExecutor NQWebUsersRestExecutor;

struct NQWebUsersRestParams {
  const char* signupUrl;
  const char* loginUrl;
  const char* updateUrl;
  const char* deleteUrl;
  const char* databasePath;
  uint8_t passwordMin;
};

struct NQWebUsersRestListeners {
  struct NQWebRequestListener signupListener;
  struct NQWebRequestListener loginListener;
  struct NQWebRequestListener updateListener;
  struct NQWebRequestListener deleteListener;
  NQSQLiteDatabase* database;
  uint8_t passwordMin;
};

struct NQWebUsersRestExecutor {
  NQWebExecutor executor;
  NQWebUsersRestListeners listeners;
};

NQ_EXPORT int NQWebUsersRestListenersInit(NQWebExecutor* executor, NQWebUsersRestListeners* listeners, const NQWebUsersRestParams* params);
NQ_EXPORT void NQWebUsersRestListenersFinalize(NQWebExecutor* executor, NQWebUsersRestListeners* listeners);

NQ_EXPORT NQWebUsersRestExecutor* NQWebUsersRestExecutorCreate(NQWebServer* server, const struct NQWebUsersRestParams*);
NQ_EXPORT void NQWebUsersRestExecutorDestroy(NQWebServer* server, NQWebUsersRestExecutor*);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_WEB_WEBUSERSRESTEXECUTOR_H */
