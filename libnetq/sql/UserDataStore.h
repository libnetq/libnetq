/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_SQL_USERDATASTORE_H
#define _LIBNETQ_SQL_USERDATASTORE_H

#include <libnetq/sql/SQLite.h>

#ifdef __cplusplus
extern "C" {
#endif

NQ_EXPORT bool NQUserDataStoreInit(NQSQLiteDatabase* database);
NQ_EXPORT void NQUserDataStoreExit(NQSQLiteDatabase* database);
NQ_EXPORT bool NQUserDataStoreSignup(NQSQLiteDatabase* database, const char* username, const char* password);
NQ_EXPORT bool NQUserDataStoreLogin(NQSQLiteDatabase* database, const char* username, const char* password);
NQ_EXPORT bool NQUserDataStoreUpdate(NQSQLiteDatabase* database, const char* username, const char* password);
NQ_EXPORT bool NQUserDataStoreDelete(NQSQLiteDatabase* database, const char* username);
NQ_EXPORT bool NQUserDataStoreUserId(NQSQLiteDatabase* database, const char* username, uint32_t* id);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_SQL_USERDATASTORE_H */
