/*
 * MIT License
 *
 * Copyright (c) 2022-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/http/HttpMultiPartParser.h"

#include <libnetq/string/String.h>
#include <libnetq/Malloc.h>
#include <libnetq/Assert.h>

enum {
  kFirstToken,
  kSecondToken,
  kBoundaryStartToken,
  kBoundaryFinishToken,
  kCRLR1Token,
  kCRLR2Token,
  kContentToken,
  kHeaderNameStartToken,
  kHeaderNameFinishToken,
  kHeaderValueStartToken,
  kHeaderValueFinishToken,
  kBodyPartToken,
  kLaterToken,
  kLatestToken,
  kErrorToken,
};

#define kDelimiterPrefix "\r\n--"
#define kDelimiterLength (4)

/*
     token          = 1*tchar
     tchar          = "!" / "#" / "$" / "%" / "&" / "'" / "*"
                    / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
                    / DIGIT / ALPHA
*/

void NQHTTPMultiPartParser_init(NQHTTPMultiPartParser* thiz, const char* boundary, NQHTTPMultiPartParserCallback callback, void* userdata)
{
  thiz->callback = callback;
  thiz->userdata = userdata;
  thiz->state = kFirstToken;

  size_t boundaryLength = 0;
  if (boundary == NULL)
    thiz->state = kErrorToken;
  else {
    for (;;) {
      char ch = boundary[boundaryLength];
      if (ch == '\0')
        break;
      if (ch == '\r' || ch == '\n' || (sizeof(thiz->delimiter) - kDelimiterLength) <= ++boundaryLength) {
        thiz->state = kErrorToken;
        boundaryLength = 0;
        break;
      }
    }
  }

  thiz->bufferLength = 0;
  thiz->delimiterIndex = 0;
  thiz->delimiterLength = boundaryLength + kDelimiterLength;
  thiz->totalBodyPartBytes = 0;

  memcpy(thiz->delimiter, kDelimiterPrefix, kDelimiterLength);
  memcpy(thiz->delimiter + kDelimiterLength, boundary, boundaryLength);
}

NQHTTPMultiPartParser* NQHTTPMultiPartParser_create(const char* boundary, NQHTTPMultiPartParserCallback callback, void* userdata)
{
  NQHTTPMultiPartParser* thiz;

  thiz = (NQHTTPMultiPartParser*)NQMalloc(sizeof(NQHTTPMultiPartParser));
  if (thiz == NULL)
    return NULL;

  NQHTTPMultiPartParser_init(thiz, boundary, callback, userdata);
  if (thiz->state == kErrorToken) {
    NQHTTPMultiPartParser_destroy(thiz);
    return NULL;
  }

  return thiz;
}

void NQHTTPMultiPartParser_destroy(NQHTTPMultiPartParser* thiz)
{
  NQHTTPMultiPartParser_finalize(thiz);
  NQFree(thiz);
}

bool NQHTTPMultiPartParser_append(NQHTTPMultiPartParser* thiz, const char* data, size_t size)
{
  if (thiz->state == kErrorToken)
    return false;

  const char* start = NULL;
  size_t index, sz;

  for (index = 0; index < size; index++) {
    char ch = data[index];
    switch (thiz->state) {
    case kFirstToken:
      if (ch == '-') {
        thiz->state = kSecondToken;
        continue;
      }
      break;

    case kSecondToken:
      if (ch == '-') {
        if (!thiz->callback || thiz->callback(thiz->userdata, kNQHTTPMultiPartBegin, "--", 2)) {
          thiz->state = kBoundaryStartToken;
          thiz->delimiterIndex = kDelimiterLength;
          continue;
        }
      }
      break;

    case kBoundaryStartToken:
      if (thiz->delimiter[thiz->delimiterIndex] == ch) {
        if (++thiz->delimiterIndex < thiz->delimiterLength)
          continue;
        if (!thiz->callback || thiz->callback(thiz->userdata, kNQHTTPMultiPartBoundary,thiz->delimiter + kDelimiterLength, thiz->delimiterLength - kDelimiterLength)) {
          thiz->state = kBoundaryFinishToken;
          thiz->delimiterIndex = 0;
          continue;
        }
      }
      break;

    case kBoundaryFinishToken:
      if (ch == '\r') {
        thiz->state = kCRLR1Token;
        continue;
      }
      if (ch == '-') {
        thiz->state = kLaterToken;
        continue;
      }
      break;

    case kCRLR1Token:
      if (ch == '\n') {
        thiz->state = kContentToken;
        continue;
      }
      break;

    case kContentToken:
      if (ch == '\r') {
        thiz->state = kCRLR2Token;
        continue;
      }
      else if (ch != ':' &&ch != '\r' && ch != '\n') { // TODO: More characters check
        thiz->state = kHeaderNameStartToken;
        start = &data[index];
        continue;
      }
      break;

    case kHeaderNameStartToken:
      if (ch == ':') {
        if (thiz->bufferLength != 0) {
          start = thiz->buffer;
          sz = thiz->bufferLength;
          thiz->bufferLength = 0;
        }
        else {
          sz = (data + index) - start;
          NQ_ASSERT(start != NULL);
        }
        if (!thiz->callback || thiz->callback(thiz->userdata, kNQHTTPMultiPartFieldName, start, sz)) {
          start = NULL;
          thiz->state = kHeaderNameFinishToken;
          continue;
        }
      }
      else if (ch != '\r' && ch != '\n') { // TODO: More characters check
        if (thiz->bufferLength == 0)
          continue;
        if (thiz->bufferLength < sizeof(thiz->buffer)) {
          thiz->buffer[thiz->bufferLength++] = ch;
          continue;
        }
      }
      break;

    case kHeaderNameFinishToken:
      if (ch == ' ') {
        continue;
      }
      else if (ch == '\r') {
        if (!thiz->callback || thiz->callback(thiz->userdata, kNQHTTPMultiPartFieldValue, "", 0)) {
          thiz->state = kHeaderValueFinishToken;
          continue;
        }
      }
      else if (ch != '\n') { // TODO: More characters check
        thiz->state = kHeaderValueStartToken;
        start = &data[index];
        continue;
      }
      break;

    case kHeaderValueStartToken:
      if (ch == '\r') {
        if (thiz->bufferLength != 0) {
          start = thiz->buffer;
          sz = thiz->bufferLength;
          thiz->bufferLength = 0;
        }
        else {
          sz = (data + index) - start;
          NQ_ASSERT(start != NULL);
        }
        while (sz != 0 && start[sz - 1] == ' ') {
          sz--;
        }
        if (!thiz->callback || thiz->callback(thiz->userdata, kNQHTTPMultiPartFieldValue, start, sz)) {
          start = NULL;
          thiz->state = kHeaderValueFinishToken;
          continue;
        }
      }
      else if (ch != '\n') { // TODO: More characters check
        if (thiz->bufferLength == 0)
          continue;
        if (thiz->bufferLength < sizeof(thiz->buffer)) {
          thiz->buffer[thiz->bufferLength++] = ch;
          continue;
        }
      }
      break;

    case kHeaderValueFinishToken:
      if (ch == '\n') {
        thiz->state = kContentToken;
        continue;
      }
      break;

    case kCRLR2Token:
      if (ch == '\n') {
        thiz->state = kBodyPartToken;
        start = &data[index + 1];
        thiz->totalBodyPartBytes = 0;
        continue;
      }
      break;

    case kBodyPartToken:
      if (thiz->delimiter[thiz->delimiterIndex] != ch) {
        if (start == NULL) {
          if (thiz->delimiterIndex != 0 && thiz->callback != NULL && !thiz->callback(thiz->userdata, kNQHTTPMultiPartBodyPart, thiz->delimiter, thiz->delimiterIndex)) {
            thiz->state = kErrorToken;
            return false;
          }
          start = &data[index];
        }
        if (thiz->delimiterIndex == 0) {
          continue;
        }
        thiz->delimiterIndex = 0;
        if (thiz->delimiter[thiz->delimiterIndex] != ch) {
          continue;
        }
      }
      if (++thiz->delimiterIndex < thiz->delimiterLength) {
        continue;
      }
      if (start == NULL) {
        sz = 0;
      }
      else {
        sz = ((data + index + 1) - start) - thiz->delimiterLength;
        thiz->totalBodyPartBytes += sz;
      }
      // thiz->totalBodyPartBytes != 0 for empty sent to client
      if ((thiz->totalBodyPartBytes != 0 && sz == 0) || !thiz->callback || thiz->callback(thiz->userdata, kNQHTTPMultiPartBodyPart, start, sz)) {
        if (!thiz->callback || thiz->callback(thiz->userdata, kNQHTTPMultiPartBoundary, thiz->delimiter + kDelimiterLength, thiz->delimiterLength - kDelimiterLength)) {
          start = NULL;
          thiz->delimiterIndex = 0;
          thiz->state = kBoundaryFinishToken;
          continue;
        }
      }
      break;

    case kLaterToken:
      if (ch == '-') {
        if (!thiz->callback || thiz->callback(thiz->userdata, kNQHTTPMultiPartFinish, "--", 2)) {
          thiz->state = kLatestToken;
          continue;
        }
      }
      break;

    case kLatestToken:
      continue;
    }

    thiz->state = kErrorToken;
    return false;
  }

  if (start != NULL) {
    if (thiz->state == kBodyPartToken) {
      sz = ((data + index) - start) - thiz->delimiterIndex;
      thiz->totalBodyPartBytes += sz;
      if (sz != 0 && thiz->callback != NULL && !thiz->callback(thiz->userdata, kNQHTTPMultiPartBodyPart, start, sz)) {
        thiz->state = kErrorToken;
        return false;
      }
    }
    else {
      size_t len = size - (start - data);
      if (len > sizeof(thiz->buffer)) {
        thiz->state = kErrorToken;
        return false;
      }
      memcpy(thiz->buffer, start, len);
      thiz->bufferLength = len;
    }
  }

  return true;
}

bool NQHTTPMultiPartParser_finish(NQHTTPMultiPartParser* thiz)
{
  if (thiz->state == kErrorToken)
    return false;

  if (thiz->state != kLatestToken) {
    thiz->state = kErrorToken;
    return false;
  }

  return true;
}
