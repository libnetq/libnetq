/*
 * MIT License
 *
 * Copyright (c) 2022-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/string/CStrToNum.h"

#include <libnetq/CType.h>
#include <libnetq/Limits.h>

#define kNQAutoBase 0

int32_t NQCStrToInt32(const char* str, char** end, int base)
{
  bool sign;
  char c = *str++;
  int32_t result = 0;

  if (c == '-') {
    sign = true;
    c = *str++;
  }
  else {
    sign = false;
    if (c == '+')
      c = *str++;
  }

  if (c != '0') {
    if (base == kNQAutoBase)
      base = 10;
  }
  else {
    c = *str++;
    if (c == 'x' || c == 'X') {
      if (base == kNQAutoBase)
        base = 16;
      else if (base != 16) {
        if (end)
          *end = (char*)(str - 1);
        return 0;
      }
      c = *str++;
    }
    else {
      if (base == kNQAutoBase)
        base = 8;
    }
  }

  for (;;) {
    if (NQIsDigit(c))
      c -= '0';
    else if (NQIsLower(c))
      c -= 'a' - 10;
    else if (NQIsUpper(c))
      c -= 'A' - 10;
    else
      break;

    if (c >= base)
      break;

    int32_t t1 = result * base + c;
    if (sign) {
      int32_t t2 = -t1;
      if (t2 > 0)
        break;
    }
    else {
      if (t1 < 0)
        break;
    }

    result = t1;
    c = *str++;
  }

  if (end)
    *end = (char*)(str - 1);

  if (sign)
    result = -result;

  return result;
}

uint32_t NQCStrToUint32(const char* str, char** end, int base)
{
  char c = *str++;
  uint32_t temp, result = 0;

  if (c != '0') {
    if (base == kNQAutoBase)
      base = 10;
  }
  else {
    c = *str++;
    if (c == 'x' || c == 'X') {
      if (base == kNQAutoBase)
        base = 16;
      else if (base != 16) {
        if (end)
          *end = (char*)(str - 1);
        return 0;
      }
      c = *str++;
    }
    else {
      if (base == kNQAutoBase)
        base = 8;
    }
  }

  for (;;) {
    if (NQIsDigit(c))
      c -= '0';
    else if (NQIsLower(c))
      c -= 'a' - 10;
    else if (NQIsUpper(c))
      c -= 'A' - 10;
    else
      break;

    if (c >= base)
      break;

    temp = result * base + c;
    if (result > temp)
      break;

    result = temp;
    c = *str++;
  }

  if (end)
    *end = (char*)(str - 1);

  return result;
}

int64_t NQCStrToInt64(const char* str, char** end, int base)
{
  bool sign;
  char c = *str++;
  int64_t result = 0;

  if (c == '-') {
    sign = true;
    c = *str++;
  }
  else {
    sign = false;
    if (c == '+')
      c = *str++;
  }

  if (c != '0') {
    if (base == kNQAutoBase)
      base = 10;
  }
  else {
    c = *str++;
    if (c == 'x' || c == 'X') {
      if (base == kNQAutoBase)
        base = 16;
      else if (base != 16) {
        if (end)
          *end = (char*)(str - 1);
        return 0;
      }
      c = *str++;
    }
    else {
      if (base == kNQAutoBase)
        base = 8;
    }
  }

  for (;;) {
    if (NQIsDigit(c))
      c -= '0';
    else if (NQIsLower(c))
      c -= 'a' - 10;
    else if (NQIsUpper(c))
      c -= 'A' - 10;
    else
      break;

    if (c >= base)
      break;

    int64_t t1 = result * base + c;
    if (sign) {
      int64_t t2 = -t1;
      if (t2 > 0)
        break;
    }
    else {
      if (t1 < 0)
        break;
    }

    result = t1;
    c = *str++;
  }

  if (end)
    *end = (char*)(str - 1);

  if (sign)
    result = -result;

  return result;
}

uint64_t NQCStrToUint64(const char* str, char** end, int base)
{
  char c = *str++;
  uint64_t temp, result = 0;

  if (c != '0') {
    if (base == kNQAutoBase)
      base = 10;
  }
  else {
    c = *str++;
    if (c == 'x' || c == 'X') {
      if (base == kNQAutoBase)
        base = 16;
      else if (base != 16) {
        if (end)
          *end = (char*)(str - 1);
        return 0;
      }
      c = *str++;
    }
    else {
      if (base == kNQAutoBase)
        base = 8;
    }
  }

  for (;;) {
    if (NQIsDigit(c))
      c -= '0';
    else if (NQIsLower(c))
      c -= 'a' - 10;
    else if (NQIsUpper(c))
      c -= 'A' - 10;
    else
      break;

    if (c >= base)
      break;

    temp = result * base + c;
    if (result > temp)
      break;

    result = temp;
    c = *str++;
  }

  if (end)
    *end = (char*)(str - 1);

  return result;
}
