/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/sql/UserDataStore.h"

#include <libnetq/Log.h>
#include <libnetq/crypto/BCrypt.h>
#include <libnetq/crypto/SecureErase.h>
#include <libnetq/Assert.h>
#include <libnetq/Limits.h>

#define USERS_TABLE "users"
#define ID_KEY     "id"
#define USER_KEY   "username"
#define HASH_KEY   "hash"
#define SALT_KEY   "salt"

#define INIT_QUERY \
  "CREATE TABLE IF NOT EXISTS " USERS_TABLE " (" \
    ID_KEY   " INTEGER PRIMARY KEY," \
    USER_KEY " TEXT UNIQUE NOT NULL," \
    HASH_KEY " BLOB NOT NULL," \
    SALT_KEY " BLOB NOT NULL" \
  ");"

#define SIGNUP_QUERY \
  "INSERT INTO " USERS_TABLE " (" USER_KEY ", " HASH_KEY ", " SALT_KEY ") VALUES(?, ?, ?);"

#define LOGIN_QUERY \
  "SELECT " HASH_KEY ", " SALT_KEY " FROM " USERS_TABLE " WHERE " USER_KEY "=?;"

#define UPDATE_QUERY \
  "UPDATE " USERS_TABLE " SET " HASH_KEY "=?, " SALT_KEY "=? WHERE " USER_KEY "=?;"

#define DELETE_QUERY \
  "DELETE FROM " USERS_TABLE " WHERE " USER_KEY "=?;"

#define USERID_QUERY \
  "SELECT " ID_KEY " FROM " USERS_TABLE " WHERE " USER_KEY "=?;"

bool NQUserDataStoreInit(NQSQLiteDatabase* database)
{
  return NQSQLiteDatabase_exec(database, INIT_QUERY);
}

void NQUserDataStoreExit(NQSQLiteDatabase* database)
{
  NQ_UNUSED_PARAM(database);
}

static inline bool signupRequest(NQSQLiteStatement* statement, const char* username, const void* salt, const void* hash)
{
  if (!NQSQLiteStatement_bindText(statement, 1, username)) {
    NQ_LOGE("Failed to bind username parameter");
    return false;
  }

  if (!NQSQLiteStatement_bindBlob(statement, 2, hash, NQ_BCRYPT_HASHSIZE)) {
    NQ_LOGE("Failed to bind password hash parameter");
    return false;
  }

  if (!NQSQLiteStatement_bindBlob(statement, 3, salt, NQ_BCRYPT_SALTSIZE)) {
    NQ_LOGE("Failed to bind password salt parameter");
    return false;
  }

  bool done;
  if (!NQSQLiteStatement_step(statement, &done)) {
    NQ_LOGE("Failed to execute signup query for user '%s'", username);
    return false;
  }

  if (!done) {
    NQ_LOGE("Signup query did not complete successfully for user '%s'", username);
    return false;
  }

  return true;
}

bool NQUserDataStoreSignup(NQSQLiteDatabase* database, const char* username, const char* password)
{
  uint8_t salt[NQ_BCRYPT_SALTSIZE];
  uint8_t hash[NQ_BCRYPT_HASHSIZE];

  if (!NQBCryptGenerateSalt(salt)) {
    NQ_LOGE("Failed to generate bcrypt salt");
    return false;
  }

  if (!NQBCryptHashPassword(password, salt, hash)) {
    NQ_LOGE("Failed to hash password for user '%s'", username);
    return false;
  }

  NQSQLiteStatement* statement = NQSQLiteDatabase_prepare(database, SIGNUP_QUERY);
  if (statement == NULL) {
    NQ_LOGE("Failed to prepare signup query");
    NQSecureErase(hash, sizeof(hash));
    NQSecureErase(salt, sizeof(salt));
    return false;
  }

  bool result = signupRequest(statement, username, salt, hash);
  NQSQLiteStatement_release(statement);
  NQSecureErase(hash, sizeof(hash));
  NQSecureErase(salt, sizeof(salt));
  return result;
}

static bool loginRequest(NQSQLiteStatement* statement, const char* username, const char* password)
{
  static const uint8_t kDummySalt[NQ_BCRYPT_SALTSIZE] = { 0 };
  static const uint8_t kDummyHash[NQ_BCRYPT_HASHSIZE] = { 0 };

  if (!NQSQLiteStatement_bindText(statement, 1, username)) {
    NQ_LOGE("Failed to bind username parameter");
    return false;
  }

  bool done;
  if (!NQSQLiteStatement_step(statement, &done)) {
    NQ_LOGE("Failed to execute login query for user '%s'", username);
    return false;
  }

  /* Always run the password hash comparison, even when the user does not
   * exist or the stored hash/salt is malformed, so that the response time
   * does not leak whether the username is valid. */
  bool userFound = !done;
  const uint8_t* hash = kDummyHash;
  const uint8_t* salt = kDummySalt;

  if (userFound) {
    size_t hashSize = NQSQLiteStatement_columnSize(statement, 0);
    const uint8_t* rowHash = (const uint8_t*)NQSQLiteStatement_columnBlob(statement, 0);
    size_t saltSize = NQSQLiteStatement_columnSize(statement, 1);
    const uint8_t* rowSalt = (const uint8_t*)NQSQLiteStatement_columnBlob(statement, 1);

    if (rowHash != NULL && hashSize == NQ_BCRYPT_HASHSIZE && rowSalt != NULL && saltSize == NQ_BCRYPT_SALTSIZE) {
      hash = rowHash;
      salt = rowSalt;
    }
    else {
      NQ_LOGE("Invalid password hash/salt for user '%s'", username);
      userFound = false;
    }
  }

  bool verified = NQBCryptVerifyPassword(password, salt, hash);

  if (!userFound) {
    NQ_LOGE("Login failed: user '%s' does not exist", username);
    return false;
  }

  if (!verified) {
    NQ_LOGE("Invalid password for user '%s'", username);
    return false;
  }

  return true;
}

bool NQUserDataStoreLogin(NQSQLiteDatabase* database, const char* username, const char* password)
{
  NQSQLiteStatement* statement = NQSQLiteDatabase_prepare(database, LOGIN_QUERY);
  if (statement == NULL) {
    NQ_LOGE("Failed to prepare login query");
    return false;
  }

  bool result = loginRequest(statement, username, password);
  NQSQLiteStatement_release(statement);
  return result;
}

static bool updateRequest(NQSQLiteDatabase* database, NQSQLiteStatement* statement, const char* username, const void* salt, const void* hash)
{
  if (!NQSQLiteStatement_bindBlob(statement, 1, hash, NQ_BCRYPT_HASHSIZE)) {
    NQ_LOGE("Failed to bind password hash parameter");
    return false;
  }

  if (!NQSQLiteStatement_bindBlob(statement, 2, salt, NQ_BCRYPT_SALTSIZE)) {
    NQ_LOGE("Failed to bind password salt parameter");
    return false;
  }

  if (!NQSQLiteStatement_bindText(statement, 3, username)) {
    NQ_LOGE("Failed to bind username parameter");
    return false;
  }

  bool done;
  if (!NQSQLiteStatement_step(statement, &done)) {
    NQ_LOGE("Failed to execute update query for user '%s'", username);
    return false;
  }

  if (!done) {
    NQ_LOGE("Update query did not complete properly for user '%s'", username);
    return false;
  }

  if (NQSQLiteStatement_changes(database) == 0) {
    NQ_LOGE("No rows updated for user '%s' (user may not exist)", username);
    return false;
  }

  return true;
}

bool NQUserDataStoreUpdate(NQSQLiteDatabase* database, const char* username, const char* password)
{
  uint8_t salt[NQ_BCRYPT_SALTSIZE];
  uint8_t hash[NQ_BCRYPT_HASHSIZE];

  if (!NQBCryptGenerateSalt(salt)) {
    NQ_LOGE("Failed to generate bcrypt salt");
    return false;
  }

  if (!NQBCryptHashPassword(password, salt, hash)) {
    NQ_LOGE("Failed to hash password for user '%s'", username);
    return false;
  }

  NQSQLiteStatement* statement = NQSQLiteDatabase_prepare(database, UPDATE_QUERY);
  if (statement == NULL) {
    NQ_LOGE("Failed to prepare update query");
    NQSecureErase(hash, sizeof(hash));
    NQSecureErase(salt, sizeof(salt));
    return false;
  }

  bool result = updateRequest(database, statement, username, salt, hash);
  NQSQLiteStatement_release(statement);
  NQSecureErase(hash, sizeof(hash));
  NQSecureErase(salt, sizeof(salt));
  return result;
}

static bool deleteRequest(NQSQLiteDatabase* database, NQSQLiteStatement* statement, const char* username)
{
  if (!NQSQLiteStatement_bindText(statement, 1, username)) {
    NQ_LOGE("Failed to bind username parameter");
    return false;
  }

  bool done;
  if (!NQSQLiteStatement_step(statement, &done)) {
    NQ_LOGE("Failed to execute delete query for user '%s'", username);
    return false;
  }

  if (!done) {
    NQ_LOGE("Delete query did not complete properly for user '%s'", username);
    return false;
  }

  if (NQSQLiteStatement_changes(database) == 0) {
    NQ_LOGE("No rows deleted for user '%s' (user may not exist)", username);
    return false;
  }

  return true;
}

bool NQUserDataStoreDelete(NQSQLiteDatabase* database, const char* username)
{
  NQSQLiteStatement* statement = NQSQLiteDatabase_prepare(database, DELETE_QUERY);
  if (statement == NULL) {
    NQ_LOGE("Failed to prepare delete query");
    return false;
  }

  bool result = deleteRequest(database, statement, username);
  NQSQLiteStatement_release(statement);
  return result;
}

static bool userIdRequest(NQSQLiteStatement* statement, const char* username, uint32_t* id)
{
  if (!NQSQLiteStatement_bindText(statement, 1, username)) {
    NQ_LOGE("Failed to bind username parameter");
    return false;
  }

  bool done;
  if (!NQSQLiteStatement_step(statement, &done)) {
    NQ_LOGE("Failed to execute user ID query");
    return false;
  }

  if (done) {
    NQ_LOGE("User '%s' not found", username);
    return false;
  }

  if (id) {
    int64_t rowId = NQSQLiteStatement_columnInt64(statement, 0);
    NQ_ASSERT(rowId >= 0 && rowId <= NQ_UINT32_MAX);
    *id = (uint32_t)rowId;
  }

  return true;
}

bool NQUserDataStoreUserId(NQSQLiteDatabase* database, const char* username, uint32_t* id)
{
  NQSQLiteStatement* statement = NQSQLiteDatabase_prepare(database, USERID_QUERY);
  if (statement == NULL) {
    NQ_LOGE("Failed to prepare user ID query");
    return false;
  }

  bool result = userIdRequest(statement, username, id);
  NQSQLiteStatement_release(statement);
  return result;
}
