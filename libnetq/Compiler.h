/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_COMPILER_H
#define _LIBNETQ_COMPILER_H

#define NQ_VERSION_DEC(major, minor, patch) ((major) * 10000 + (minor) * 100 + (patch))
#define NQ_VERSION_HEX(major, minor, patch) ((major) * 0x10000 + (minor) * 0x100 + (patch))

#if defined(__GNUC__)
#define NQ_COMPILER_GCC 1
#define NQ_GCC_VERSION NQ_VERSION_DEC(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#define NQ_GCC_VERSION_AT_LEAST(major, minor, patch) (NQ_GCC_VERSION >= NQ_VERSION_DEC(major, minor, patch))
#else
#define NQ_GCC_VERSION_AT_LEAST(major, minor, patch) 0
#endif

#if defined(__clang__)
#define NQ_COMPILER_CLANG 1
#endif

#if defined(__MINGW32__)
#define NQ_COMPILER_MINGW 1
#endif

#if defined(NQ_COMPILER_MINGW) && (defined(__MINGW64_VERSION_MAJOR) || defined(__MINGW64__))
#define NQ_COMPILER_MINGW64 1
#endif

#if defined(__CYGWIN32__) || defined(__CYGWIN__)
#define NQ_COMPILER_CYGWIN 1
#endif

#if defined(_MSC_VER)
#define NQ_COMPILER_MSVC 1
#endif

#if defined(__has_attribute)
#define NQ_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
#define NQ_HAS_ATTRIBUTE(x) 0
#define __has_attribute(x) 0
#endif

#if defined(__has_builtin)
#define NQ_HAS_BUILTIN(x) __has_builtin(x)
#else
#define NQ_HAS_BUILTIN(x) 0
#define __has_builtin(x) 0
#endif

#if defined(__has_include)
#define NQ_HAS_INCLUDE(x) __has_include(x)
#else
#define NQ_HAS_INCLUDE(x) 0
#define __has_include(x) 0
#endif

#if !defined(NQ_LIKELY) && defined(NQ_COMPILER_GCC)
#define NQ_LIKELY(x) __builtin_expect(!!(x), 1)
#endif

#if !defined(NQ_LIKELY)
#define NQ_LIKELY(x) (x)
#endif

#if !defined(NQ_UNLIKELY) && defined(NQ_COMPILER_GCC)
#define NQ_UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

#if !defined(NQ_UNLIKELY)
#define NQ_UNLIKELY(x) (x)
#endif

#if !defined(NQ_ALWAYS_INLINE) \
    && (defined(NQ_COMPILER_GCC) || defined(NQ_COMPILER_CLANG)) \
    && !defined(NQ_COMPILER_MINGW)
#define NQ_ALWAYS_INLINE inline __attribute__((__always_inline__))
#endif

#if !defined(NQ_ALWAYS_INLINE) && defined(NQ_COMPILER_MSVC)
#define NQ_ALWAYS_INLINE __forceinline
#endif

#if !defined(NQ_ALWAYS_INLINE)
#define NQ_ALWAYS_INLINE inline
#endif

#ifdef NQ_COMPILER_GCC
#define NQ_NEVER_INLINE __attribute__((__noinline__))
#elif defined(NQ_COMPILER_MSVC) && _MSC_VER >= 1300
#define NQ_NEVER_INLINE __declspec(noinline)
#else
#define NQ_NEVER_INLINE
#endif

#define NQ_NOINLINE NQ_NEVER_INLINE
#define NQ_INLINE __inline

#ifndef NQ_NORETURN
#if NQ_HAS_ATTRIBUTE(__noreturn__) || defined(NQ_COMPILER_GCC) || defined(NQ_COMPILER_CLANG)
#define NQ_NORETURN __attribute((__noreturn__))
#elif defined(NQ_COMPILER_MSVC)
#define NQ_NORETURN __declspec(noreturn)
#else
#define NQ_NORETURN
#endif
#endif

#if !defined(NQ_RETURNS_NONNULL) && defined(NQ_COMPILER_GCC) && defined(NQ_COMPILER_CLANG)
# define NQ_RETURNS_NONNULL __attribute__((returns_nonnull))
#endif

#if !defined(NQ_RETURNS_NONNULL)
# define NQ_RETURNS_NONNULL
#endif

#if !defined(NQ_PURE_FUNCTION) && defined(NQ_COMPILER_GCC)
#define NQ_PURE_FUNCTION __attribute__((__pure__))
#endif

#if !defined(NQ_PURE_FUNCTION)
#define NQ_PURE_FUNCTION
#endif

#ifdef NQ_COMPILER_GCC
#define NQ_ALLOW_UNUSED __attribute__((unused))
#else
#define NQ_ALLOW_UNUSED
#endif

#if !defined(NQ_UNUSED_PARAM) && defined(NQ_COMPILER_MSVC)
#define NQ_UNUSED_PARAM(variable) (void)&variable
#endif

#if !defined(NQ_UNUSED_PARAM)
#define NQ_UNUSED_PARAM(variable) (void)variable
#endif

#if !defined(NQ_WARN_UNUSED_RETURN) && defined(NQ_COMPILER_GCC) && defined(NQ_COMPILER_CLANG)
#define NQ_WARN_UNUSED_RETURN __attribute__((__warn_unused_result__))
#endif

#if !defined(NQ_WARN_UNUSED_RETURN)
#define NQ_WARN_UNUSED_RETURN
#endif

#if defined(NQ_COMPILER_GCC)
#define NQ_PRETTY_FUNCTION __PRETTY_FUNCTION__
#elif defined(NQ_COMPILER_MSVC)
#define NQ_PRETTY_FUNCTION __FUNCDNAME__
#else
#define NQ_PRETTY_FUNCTION __FUNCTION__
#endif

#if defined(NQ_COMPILER_GCC) && !defined(__OBJC__)
#define NQ_ATTRIBUTE_PRINTF(formatIndex, argumentsIndex) __attribute__((__format__(printf, formatIndex, argumentsIndex)))
#if NQ_GCC_VERSION_AT_LEAST(5,0,0)
#define NQ_ATTRIBUTE_WPRINTF(formatIndex, argumentsIndex) __attribute__((__format__(wprintf, formatIndex, argumentsIndex)))
#else
#define NQ_ATTRIBUTE_WPRINTF(formatIndex, argumentsIndex)
#endif
#else
#define NQ_ATTRIBUTE_PRINTF(formatIndex, argumentsIndex)
#define NQ_ATTRIBUTE_WPRINTF(formatIndex, argumentsIndex)
#endif

#if  defined(__SSE__) \
  || defined(__SSE_MATH__) \
  || (defined(_M_IX86_FP) && (_M_IX86_FP >= 1))
#define NQ_HAS_COMPILER_SSE
#endif

#if  defined(__SSE2__) \
  || defined(__SSE2_MATH__) \
  || (defined(_M_IX86_FP) && (_M_IX86_FP >= 2))
#define NQ_HAS_COMPILER_SSE2
#endif

#define NQ_FILE __FILE__
#define NQ_LINE __LINE__
#define NQ_DATE __DATE__
#define NQ_TIME __TIME__
#define NQ_COUNTER __COUNTER__
#define NQ_DATETIME (NQ_DATE " " NQ_TIME " PST")

#define NQ_TODO 0
#define NQ_FIXME 1

#if defined(NDEBUG)
#ifdef _DEBUG
#error Alrady defined NDEBUG
#endif
#define NQ_DEBUG 0
#define NQ_NDEBUG 1
#else
#define NQ_DEBUG 1
#define NQ_NDEBUG 0
#endif

#ifdef __cplusplus
#define NQ_USE_CPP 1
#endif

#endif /* _LIBNETQ_COMPILER_H */
