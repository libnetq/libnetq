/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_WEB_WEBSOCKETCALCACCEPT_H
#define _LIBNETQ_WEB_WEBSOCKETCALCACCEPT_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

NQ_EXPORT int NQWebSocketCalcAccept(const char* key, char* buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_WEB_WEBSOCKETCALCACCEPT_H */
