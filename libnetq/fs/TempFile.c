/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/fs/TempFile.h"

#include <libnetq/Env.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/fs/Mkdir.h>
#include <libnetq/fs/Path.h>
#include <libnetq/fs/Stat.h>
#include <libnetq/random/CryptoRandom.h>
#include <libnetq/string/String.h>

#if defined(NQ_OS_WINDOWS)
# include <windows.h>
#elif defined(NQ_OS_UNIX)
# include <fcntl.h>
# include <unistd.h>
# include <sys/stat.h>
#endif

#if defined(NQ_OS_ANDROID)
# define DEFAULT_TEMP_DIRECTORY "/data/local/tmp"
#else
# define DEFAULT_TEMP_DIRECTORY "/tmp"
#endif

/* As many tries as there are names of three characters */
#define ATTEMPTS (62 * 62 * 62)

static const char letters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

#define LETTER_COUNT 62
/* Bytes from here on are not used, or the first letters would come up
 * more often than the last */
#define RANDOM_LIMIT (256 - 256 % LETTER_COUNT)

static int random_letters(char* out, size_t count)
{
  uint8_t bytes[64];
  size_t have = 0, used = 0;
  while (count > 0) {
    if (used == have) {
      int ret = NQGetCryptoRandom(bytes, sizeof(bytes));
      if (ret != 0)
        return ret < 0 ? ret : -NQ_EIO;
      have = sizeof(bytes);
      used = 0;
    }
    uint8_t byte = bytes[used++];
    if (byte < RANDOM_LIMIT) {
      *out++ = letters[byte % LETTER_COUNT];
      count--;
    }
  }
  return 0;
}

static bool is_taken(int error)
{
#if defined(NQ_OS_WINDOWS)
  /* CreateFileW and CreateDirectoryW do not agree on it */
  return error == -ERROR_FILE_EXISTS || error == -ERROR_ALREADY_EXISTS;
#else
  return error == -NQ_EEXIST;
#endif
}

/* Creates the file only if nothing has the name, even a dangling link */
static int create_file(const char* path)
{
#if defined(NQ_OS_WINDOWS)
  NQWinPath winpath;
  if (!NQWinPathInit(&winpath, path))
    return -NQ_ENOMEM;
  HANDLE file = CreateFileW(winpath.characters, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW,
                            FILE_ATTRIBUTE_NORMAL, NULL);
  int error = file == INVALID_HANDLE_VALUE ? -NQGetLastError() : 0;
  NQWinPathFinalize(&winpath);
  if (error == 0)
    CloseHandle(file);
  return error;

#elif defined(NQ_OS_UNIX)
  int fd = open(path, O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
  if (fd < 0)
    return -NQGetLastError();
  return close(fd) == 0 ? 0 : -NQGetLastError();

#else
  NQ_UNUSED_PARAM(path);
  return -NQ_ENOTSUPP;
#endif
}

/* Is the name free? A link has it too, wherever it points, so links are
 * not followed. Returns 0, -NQ_EEXIST, or why it cannot be told. */
static int check_free(const char* path)
{
#if defined(NQ_OS_WINDOWS)
  NQStat st;
  int ret = NQGetStat(path, &st); /* does not follow links here */
  if (ret == 0)
    return -NQ_EEXIST;
  return ret == -ERROR_FILE_NOT_FOUND || ret == -ERROR_PATH_NOT_FOUND ? 0 : ret;

#elif defined(NQ_OS_UNIX)
  struct stat st;
  if (lstat(path, &st) == 0)
    return -NQ_EEXIST;
  int error = NQGetLastError();
  return error == NQ_ENOENT ? 0 : -error;

#else
  NQ_UNUSED_PARAM(path);
  return -NQ_ENOTSUPP;
#endif
}

int NQMakeTempPath(char* path, size_t suffixLength, size_t count, NQTempKind kind)
{
  size_t len = NQStrlen(path);
  if (count == 0 || suffixLength > len || count > len - suffixLength)
    return -NQ_EINVAL;
  char* name = path + len - suffixLength - count;

  for (unsigned attempt = 0; attempt < ATTEMPTS; attempt++) {
    int ret = random_letters(name, count);
    if (ret != 0)
      return ret;

    if (kind == NQ_TEMP_FILE)
      ret = create_file(path);
    else if (kind == NQ_TEMP_DIRECTORY)
      ret = NQMkdir(path, 0700);
    else
      ret = check_free(path);
    if (!is_taken(ret))
      return ret;
  }
  return -NQ_EEXIST;
}

int NQGetTempDirectory(char* buffer, size_t n)
{
  if (n == 0)
    return -NQ_ENAMETOOLONG;
  buffer[0] = '\0';

  int len = NQEnvGet("TMPDIR", buffer, n);
  if (len > 0 && (size_t)len < n)
    return len;

#if defined(NQ_OS_WINDOWS)
  WCHAR wide[MAX_PATH + 1];
  DWORD length = GetTempPathW(NQ_ARRAY_LENGTH(wide), wide);
  if (length == 0)
    return -NQGetLastError();
  if (length >= NQ_ARRAY_LENGTH(wide))
    return -NQ_ENAMETOOLONG;
  size_t size = NQPathFrom(buffer, n, wide);
  if (size == 0 || size >= n)
    return -NQ_ENAMETOOLONG;
  /* it says "C:\Temp\": the end is not part of the name */
  if (size > 1 && buffer[size - 1] == '/' && buffer[size - 2] != ':')
    buffer[--size] = '\0';
  return (int)size;

#else
  size_t size = sizeof(DEFAULT_TEMP_DIRECTORY) - 1;
  if (size >= n)
    return -NQ_ENAMETOOLONG;
  NQMemcpy(buffer, DEFAULT_TEMP_DIRECTORY, size + 1);
  return (int)size;
#endif
}
