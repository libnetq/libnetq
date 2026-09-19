/*
 * MIT License
 *
 * Copyright (c) 2021-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_ENDIAN_H
#define _LIBNETQ_ENDIAN_H

#include <libnetq/CPU.h>
#include <libnetq/ByteOrder.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(NQ_CPU_BIG_ENDIAN)
#define NQHtole16(x) NQByteSwap16(x)
#define NQLe16toh(x) NQByteSwap16(x)
#define NQHtobe16(x) (x)
#define NQBe16toh(x) (x)
#define NQHtole32(x) NQByteSwap32(x)
#define NQLe32toh(x) NQByteSwap32(x)
#define NQHtobe32(x) (x)
#define NQBe32toh(x) (x)
#define NQHtole64(x) NQByteSwap64(x)
#define NQLe64toh(x) NQByteSwap64(x)
#define NQHtobe64(x) (x)
#define NQBe64toh(x) (x)

#else
#define NQHtole16(x) (x)
#define NQLe16toh(x) (x)
#define NQHtobe16(x) NQByteSwap16(x)
#define NQBe16toh(x) NQByteSwap16(x)
#define NQHtole32(x) (x)
#define NQLe32toh(x) (x)
#define NQHtobe32(x) NQByteSwap32(x)
#define NQBe32toh(x) NQByteSwap32(x)
#define NQHtole64(x) (x)
#define NQLe64toh(x) (x)
#define NQHtobe64(x) NQByteSwap64(x)
#define NQBe64toh(x) NQByteSwap64(x)

#endif

#define NQNtohs NQBe16toh
#define NQHtons NQHtobe16

#define NQHostToLE16 NQHtole16
#define NQLEToHost16 NQLe16toh
#define NQHostToBE16 NQHtobe16
#define NQBEToHost16 NQBe16toh
#define NQHostToLE32 NQHtole32
#define NQLEToHost32 NQLe32toh
#define NQHostToBE32 NQHtobe32
#define NQBEToHost32 NQBe32toh
#define NQHostToLE64 NQHtole64
#define NQLEToHost64 NQLe64toh
#define NQHostToBE64 NQHtobe64
#define NQBEToHost64 NQBe64toh

NQ_EXPORT bool NQIsLittleEndian(void);
NQ_EXPORT bool NQIsBigEndian(void);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_ENDIAN_H */
