/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/net/WakeOnLan.h"

#include <libnetq/String.h>

void NQWOLPacket_init(NQWOLPacket* packet, const NQMACAddress* address)
{
  uint8_t* ptr = packet->data;

  memset(ptr, 0xFF, sizeof(address->data));
  ptr += sizeof(address->data);

  for (size_t i = 0; i < 16; i++, ptr += sizeof(address->data))
    memcpy(ptr, address->data, sizeof(address->data));
}
