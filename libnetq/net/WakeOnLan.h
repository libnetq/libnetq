/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_NET_WAKEONLAN_H
#define _LIBNETQ_NET_WAKEONLAN_H

#include <libnetq/Network.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_WOL_DEFAULT_PORT (60000)
#define NQ_WOL_PACKET_SIZE (102)

typedef struct NQWOLPacket NQWOLPacket;

struct NQWOLPacket {
  uint8_t data[NQ_WOL_PACKET_SIZE];
};

static inline const uint8_t* NQWOLPacket_data(const NQWOLPacket* thiz)
{
  return thiz->data;
}

static inline size_t NQWOLPacket_size(const NQWOLPacket* thiz)
{
  return NQ_WOL_PACKET_SIZE;
}

NQ_EXPORT void NQWOLPacket_init(NQWOLPacket*, const NQMACAddress* address);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_NET_WAKEONLAN_H */
