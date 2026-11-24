/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/backend/win32/Socket.h"

#ifdef NQ_OS_WINDOWS

# include <winsock2.h>

#include <libnetq/Assert.h>
#include <libnetq/sync/Once.h>

#define WSA_VERSION MAKEWORD(2, 2)

static WSADATA s_wsadata;
static NQOnce s_once = NQ_ONCE_INIT;

static void startupInit(void)
{
  int status = WSAStartup(WSA_VERSION, &s_wsadata);
  NQ_ASSERT_UNUSED(status, !status);
}

void NQWinSocketInitialize(void)
{
  NQOnce_call(&s_once, &startupInit);
}

void NQWinSocketCleanup(void)
{
  int status = WSACleanup();
  NQ_ASSERT_UNUSED(status, !status);
}

#endif /* NQ_OS_WINDOWS */
