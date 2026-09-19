/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/sql/UrlDataStore.h"

#include <libnetq/Assert.h>
#include <libnetq/Log.h>
#include <libnetq/MinMax.h>
#include <libnetq/fs/Path.h>
#include <libnetq/string/StringPrint.h>
#include <libnetq/crypto/BCrypt.h>

#define FILES_TABLE  "files"
#define ID_KEY       "id"
#define USER_KEY     "user"
#define DIR_KEY      "dir"
#define BASE_KEY     "base"
#define VALUE_KEY    "value"

#define INIT_QUERY \
  "CREATE TABLE IF NOT EXISTS " FILES_TABLE " (" \
    ID_KEY    " INTEGER PRIMARY KEY," \
    USER_KEY  " INTEGER NOT NULL," \
    DIR_KEY   " TEXT NOT NULL," \
    BASE_KEY  " TEXT NOT NULL," \
    VALUE_KEY " TEXT," \
    "UNIQUE(" USER_KEY ", " DIR_KEY  ", " BASE_KEY ")" \
  ");"

#define READ_QUERY \
  "SELECT " VALUE_KEY " FROM " FILES_TABLE \
    " WHERE " USER_KEY  "=?" \
    " AND "   DIR_KEY   "=?" \
    " AND "   BASE_KEY  "=?" \
    " AND "   VALUE_KEY " IS NOT NULL;"

#define WRITE_QUERY1 \
  "INSERT OR REPLACE INTO " FILES_TABLE " (" USER_KEY ", " DIR_KEY ", " BASE_KEY ", " VALUE_KEY ") SELECT ?, ?, ?, ?;"

#define WRITE_QUERY2 \
  "INSERT OR REPLACE INTO " FILES_TABLE " (" USER_KEY ", " DIR_KEY ", " BASE_KEY ", " VALUE_KEY ") SELECT ?, ?, ?, ?" \
  " WHERE EXISTS (" \
    " SELECT 1 FROM " FILES_TABLE \
    " WHERE " USER_KEY  "=?" \
    " AND "   DIR_KEY   "=?" \
    " AND "   BASE_KEY  "=?" \
    " AND "   VALUE_KEY " IS NULL" \
  ");"

#define LIST_QUERY \
  "SELECT " BASE_KEY ", " VALUE_KEY " FROM " FILES_TABLE \
    " WHERE " USER_KEY "=?" \
    " AND " DIR_KEY    "=?;"

bool NQUrlDataStoreInit(NQSQLiteDatabase* database)
{
  return NQSQLiteDatabase_exec(database, INIT_QUERY);
}

void NQUrlDataStoreExit(NQSQLiteDatabase* database)
{
  NQ_UNUSED_PARAM(database);
}

static int readRequest(NQSQLiteStatement* statement, int32_t userId, const NQPathInfo* path, char* buf, size_t len)
{
  if (!NQSQLiteStatement_bindInt32(statement, 1, userId)) {
    NQ_LOGE("Failed to bind user parameter");
    return -1;
  }

  if (!NQSQLiteStatement_bindText2(statement, 2, path->dirname.characters, path->dirname.length)) {
    NQ_LOGE("Failed to bind dirname parameter");
    return -1;
  }

  if (!NQSQLiteStatement_bindText2(statement, 3, path->basename.characters, path->basename.length)) {
    NQ_LOGE("Failed to bind basename parameter");
    return -1;
  }

  bool done;
  if (!NQSQLiteStatement_step(statement, &done)) {
    NQ_LOGE("Failed to execute statement");
    return -1;
  }

  if (done) {
    NQ_LOGE("No URL data found for user=%d, path=%.*s", userId, (int)path->path.length, path->path.characters);
    return -1;
  }

  size_t size = NQSQLiteStatement_columnSize(statement, 0);
  const char* data = NQSQLiteStatement_columnText(statement, 0);
  if (data == NULL) {
    NQ_LOGE("Failed to read content for user=%d, path=%.*s", userId, (int)path->path.length, path->path.characters);
    return -1;
  }

  memcpy(buf, data, NQGetMin(len, size));
  return (int)size;
}

int NQUrlDataStoreRead(NQSQLiteDatabase* database, int32_t userId, const char* path, char* buf, size_t len)
{
  NQPathInfo pathInfo;
  if (!NQPathInfoParse(path, &pathInfo) || !pathInfo.isAbsolute || pathInfo.isDirOnly || !pathInfo.isNormalize || pathInfo.basename.length == 0) {
    NQ_LOGE("Invalid path: %s", path);
    return false;
  }

  NQSQLiteStatement* statement = NQSQLiteDatabase_prepare(database, READ_QUERY);
  if (statement == NULL) {
    NQ_LOGE("Failed to prepare path read statement");
    return false;
  }

  int result = readRequest(statement, userId, &pathInfo, buf, len);
  NQSQLiteStatement_release(statement);
  return result;
}

static int writeRequest(NQSQLiteStatement* statement, int32_t userId, const NQPathInfo* path, const char* buf, size_t len)
{
  if (!NQSQLiteStatement_bindInt32(statement, 1, userId)) {
    NQ_LOGE("Failed to bind user parameter");
    return false;
  }

  if (!NQSQLiteStatement_bindText2(statement, 2, path->dirname.characters, path->dirname.length)) {
    NQ_LOGE("Failed to bind dirname parameter");
    return false;
  }

  if (!NQSQLiteStatement_bindText2(statement, 3, path->basename.characters, path->basename.length)) {
    NQ_LOGE("Failed to bind basename parameter");
    return false;
  }

  if (buf == NULL) {
    NQ_ASSERT_WITH_MESSAGE(len == 0, "Invalid NULL write with non-zero length");
    if (!NQSQLiteStatement_bindNull(statement, 4)) {
      NQ_LOGE("Failed to bind value parameter");
      return -1;
    }
  }
  else {
    if (!NQSQLiteStatement_bindText2(statement, 4, buf, len)) {
      NQ_LOGE("Failed to bind value parameter");
      return -1;
    }
  }

  if (path->dirname.length > 1) {
    NQPathInfo dirInfo;
    NQPathInfoParse2(path->dirname.characters, path->dirname.length, &dirInfo);
    if (!NQSQLiteStatement_bindInt32(statement, 5, userId)) {
      NQ_LOGE("Failed to bind user parameter");
      return false;
    }
    if (!NQSQLiteStatement_bindText2(statement, 6, dirInfo.dirname.characters, dirInfo.dirname.length)) {
      NQ_LOGE("Failed to bind basename parameter");
      return false;
    }
    if (!NQSQLiteStatement_bindText2(statement, 7, dirInfo.basename.characters, dirInfo.basename.length)) {
      NQ_LOGE("Failed to bind basename parameter");
      return false;
    }
  }

  bool done;
  if (!NQSQLiteStatement_step(statement, &done)) {
    NQ_LOGE("Failed to execute statement");
    return -1;
  }

  if (!done) {
    NQ_LOGE("Path write query did not complete properly");
    return -1;
  }

  return (int)len;
}

static int urldataStoreWrite(NQSQLiteDatabase* database, int32_t userId, const char* path, const char* buf, size_t len)
{
  NQPathInfo pathInfo;
  if (!NQPathInfoParse(path, &pathInfo) || !pathInfo.isAbsolute || pathInfo.isDirOnly || !pathInfo.isNormalize || pathInfo.basename.length == 0) {
    NQ_LOGE("Invalid path: %s", path);
    return -1;
  }

  NQSQLiteStatement* statement = NQSQLiteDatabase_prepare(database, pathInfo.dirname.length == 1 ? WRITE_QUERY1 : WRITE_QUERY2);
  if (statement == NULL) {
    NQ_LOGE("Failed to prepare path write statement");
    return -1;
  }

  int result = writeRequest(statement, userId, &pathInfo, buf, len);
  NQSQLiteStatement_release(statement);
  return result;
}

int NQUrlDataStoreWrite(NQSQLiteDatabase* database, int32_t userId, const char* path, const char* buf, size_t len)
{
  if (buf == 0) {
    NQ_LOGE("Buffer argument is null");
    return -1;
  }

  return urldataStoreWrite(database, userId, path, buf, len);
}

bool NQUrlDataStoreMkdir(NQSQLiteDatabase* database, int32_t userId, const char* path)
{
  return urldataStoreWrite(database, userId, path, NULL, 0) == 0;
}

static bool listRequest(NQSQLiteStatement* statement, int32_t userId, const NQPathInfo* path, NQUrlDataStoreCallback callback, void* userdata)
{
  if (!NQSQLiteStatement_bindInt32(statement, 1, userId)) {
    NQ_LOGE("Failed to bind userId parameter");
    return false;
  }

  if (!NQSQLiteStatement_bindText2(statement, 2, path->path.characters, path->path.length)) {
    NQ_LOGE("Failed to bind path parameter");
    return false;
  }

  for (;;) {
    bool done;
    if (!NQSQLiteStatement_step(statement, &done)) {
      NQ_LOGE("Failed to execute statement");
      return false;
    }
    if (done)
      break;
    const char* name = NQSQLiteStatement_columnText(statement, 0);
    int type = NQSQLiteStatement_columnType(statement, 0);
    const char* val = NULL;
    size_t len = 0;
    if (type != kNQSQLiteValueNull) {
      val = NQSQLiteStatement_columnText(statement, 1);
      len = NQSQLiteStatement_columnSize(statement, 1);
    }
    if (!callback(userdata, name, val, len))
      break;
  }

  return true;
}

bool NQUrlDataStoreList(NQSQLiteDatabase* database, int32_t userId, const char* path, NQUrlDataStoreCallback callback, void* userdata)
{
  NQPathInfo pathInfo;
  if (!NQPathInfoParse(path, &pathInfo) || !pathInfo.isAbsolute || !pathInfo.isNormalize) {
    NQ_LOGE("Invalid path: %s", path);
    return false;
  }

  NQSQLiteStatement* statement = NQSQLiteDatabase_prepare(database, LIST_QUERY);
  if (statement == NULL) {
    NQ_LOGE("Failed to prepare list statement");
    return false;
  }

  bool success = listRequest(statement, userId, &pathInfo, callback, userdata);
  NQSQLiteStatement_release(statement);
  return success;
}
