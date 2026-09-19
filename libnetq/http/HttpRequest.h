/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_HTTP_HTTPREQUEST_H
#define _LIBNETQ_HTTP_HTTPREQUEST_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NQHttpRequest NQHttpRequest;
typedef struct NQHttpRequestHeaderIter NQHttpRequestHeaderIter;
typedef int (*NQHttpRequestWriteCallback) (void* userdata, const void* data, size_t size);

NQ_EXPORT NQHttpRequest* NQHttpRequest_create(NQHttpRequestWriteCallback callback, void* userdata);
NQ_EXPORT void NQHttpRequest_release(NQHttpRequest*);

NQ_EXPORT int NQHttpRequest_performSync(NQHttpRequest*);
NQ_EXPORT const char* NQHttpRequest_lastErrorMessage(NQHttpRequest*);

NQ_EXPORT bool NQHttpRequest_setUrl(NQHttpRequest*, const char* url);
NQ_EXPORT bool NQHttpRequest_setMethod(NQHttpRequest*, const char* method);
NQ_EXPORT bool NQHttpRequest_setPostData(NQHttpRequest*, const void* data, size_t size);
NQ_EXPORT bool NQHttpRequest_setFollowLocation(NQHttpRequest*, bool value);
NQ_EXPORT bool NQHttpRequest_setTimeoutMs(NQHttpRequest* thiz, int64_t timeoutMs);
NQ_EXPORT bool NQHttpRequest_addHeader(NQHttpRequest*, const char* name, const char* value);

NQ_EXPORT int NQHttpRequest_responseStatusCode(NQHttpRequest*);
NQ_EXPORT const char* NQHttpRequest_responseReasonText(NQHttpRequest*);
NQ_EXPORT bool NQHttpRequest_responseTimeMs(NQHttpRequest*, int64_t* result);
NQ_EXPORT size_t NQHttpRequest_responseHeaderCount(NQHttpRequest*);
NQ_EXPORT const char* NQHttpRequest_responseHeaderNameAt(NQHttpRequest*, size_t index);
NQ_EXPORT const char* NQHttpRequest_responseHeaderValueAt(NQHttpRequest*, size_t index);

NQ_EXPORT NQHttpRequestHeaderIter* NQHttpRequest_responseHeaderFirst(NQHttpRequest*);
NQ_EXPORT NQHttpRequestHeaderIter* NQHttpRequest_responseHeaderNext(NQHttpRequest*, NQHttpRequestHeaderIter* iter);
NQ_EXPORT const char* NQHttpRequestHeaderIter_name(NQHttpRequestHeaderIter* iter);
NQ_EXPORT const char* NQHttpRequestHeaderIter_value(NQHttpRequestHeaderIter* iter);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_HTTP_HTTPREQUEST_H */
