/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_AUTORELEASECF_H
#define _LIBNETQ_AUTORELEASECF_H

#ifdef __cplusplus
extern "C" {
#endif

extern void* objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void* pool);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_AUTORELEASECF_H */
