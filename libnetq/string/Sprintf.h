/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_STRING_SPRINTF_H
#define _LIBNETQ_STRING_SPRINTF_H

#include <libnetq/Basic.h>

#ifdef NQ_OS_KERNEL
#include <linux/stdarg.h>
#include <linux/sprintf.h>
#else
#include <stdio.h>
#endif

#define NQSprintf sprintf
#define NQSnprintf snprintf

#endif /* _LIBNETQ_STRING_SPRINTF_H */

