/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_OS_H
#define _LIBNETQ_OS_H

#if (defined(__linux__) || defined(__linux)) && defined(__KERNEL__)
# define NQ_OS_KERNEL 1
#else

# if defined(ANDROID) || defined(__ANDROID__)
#  define NQ_OS_ANDROID 1
#  include <android/api-level.h>
#endif

# if defined(__linux__) || defined(__linux) || defined(NQ_OS_ANDROID)
#  define NQ_OS_LINUX 1
# endif

# if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
#  define NQ_OS_WINDOWS 1
# endif

# if defined(__APPLE__) || defined(__MACH__)
#  define NQ_OS_DARWIN 1
#  if defined(__APPLE__)
#   include <TargetConditionals.h>
#   if TARGET_OS_OSX
#    define NQ_OS_MACOSX 1
#   elif TARGET_OS_IPHONE
#    define NQ_OS_IOS 1
#   endif
#  endif
#  endif

# if defined(NQ_OS_DARWIN) || defined(NQ_OS_LINUX) \
 || defined(unix) || defined(__unix) || defined(__unix__)
#  define NQ_OS_POSIX 1
#  define NQ_OS_UNIX 1
# endif

# if !(defined(NQ_OS_WINDOWS) || defined(NQ_OS_POSIX))
#  define NQ_OS_UNKNOWN 1
# endif

#endif

#endif /* _LIBNETQ_OS_H */
