/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_CPU_RISCV_H
#define _LIBNETQ_CPU_RISCV_H

#define NQ_FOR_EACH_CPU_RISCV(macro) \
  macro(__riscv)        \
  macro(__riscv_xlen)   \
  macro(__riscv64)      \
  macro(__riscv_atomic) \
  macro(__riscv_mul)    \
  macro(__riscv_muldiv)  \
  macro(__riscv_flen)

#if  defined (__riscv)        \
  || defined (__riscv_xlen)   \
  || defined (__riscv64)      \
  || defined (__riscv_atomic) \
  || defined (__riscv_mul)    \
  || defined (__riscv_muldiv) \
  || defined (__riscv_flen)

#define NQ_CPU_RISCV 1
#define NQ_CPU_NAME "RISC-V"

/* RISC-V is little-endian by default; GCC's -mbig-endian is only visible through __BYTE_ORDER__ */
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define NQ_CPU_BIG_ENDIAN 1
#else
#define NQ_CPU_LITTLE_ENDIAN 1
#endif

#if defined(__riscv_xlen)
# if __riscv_xlen == 64
#  define NQ_CPU_64BIT 1
# elif __riscv_xlen == 32
#  define NQ_CPU_32BIT 1
# else
#  error Not supported RISC-V bit size
# endif
#elif defined(__riscv64)
# define NQ_CPU_64BIT 1
#endif

/* psABI name: integer ABI (lp64/ilp32, "e" for the RV32E/RV64E reduced register set) + float ABI suffix */
#if defined(__riscv_abi_rve)
# define NQ_CPU_RISCV_ABI_E "e"
#else
# define NQ_CPU_RISCV_ABI_E ""
#endif
#if defined(__riscv_float_abi_double)
# define NQ_CPU_RISCV_ABI_FLOAT "d"
#elif defined(__riscv_float_abi_single)
# define NQ_CPU_RISCV_ABI_FLOAT "f"
#elif defined(__riscv_float_abi_quad)
# define NQ_CPU_RISCV_ABI_FLOAT "q"
#else
# define NQ_CPU_RISCV_ABI_FLOAT ""
#endif
#if defined(NQ_CPU_64BIT)
# define NQ_CPU_ABI "riscv64-lp64" NQ_CPU_RISCV_ABI_E NQ_CPU_RISCV_ABI_FLOAT
#elif defined(NQ_CPU_32BIT)
# define NQ_CPU_ABI "riscv32-ilp32" NQ_CPU_RISCV_ABI_E NQ_CPU_RISCV_ABI_FLOAT
#else
# define NQ_CPU_ABI "riscv"
#endif

#endif

#endif /* _LIBNETQ_CPU_RISCV_H */
