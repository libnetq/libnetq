#
# MIT License
#
# Copyright (c) 2022-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
#
# Permission is granted to use, copy, modify, and distribute this software
# under the MIT License. See LICENSE file for details.
#

if (TARGET PCRE2::PCRE2)
  set(PCRE2_FIND_QUIETLY TRUE)
  set(PCRE2_FOUND TRUE)
  return ()
endif ()

if (PCRE2_INCLUDE_DIR AND PCRE2_LIBRARY)
  set(PCRE2_FIND_QUIETLY TRUE)
endif ()

find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
  pkg_check_modules(PC_PCRE2 PCRE2)
  set(PCRE2_DEFINITIONS ${PC_PCRE2_CFLAGS_OTHER})
endif ()

find_path(PCRE2_INCLUDE_DIR
  NAMES pcre2.h
  PATHS
    ${PC_PCRE2_INCLUDEDIR}
    ${PC_PCRE2_INCLUDE_DIR}
  )

find_library(PCRE2_LIBRARY
  NAMES
    pcre2-8 pcre2-8-static pcre2-8-staticd
  PATHS
    ${PC_PCRE2_LIBDIR}
    ${PC_PCRE2_LIBRARIES_DIRS}
  )

mark_as_advanced(PCRE2_INCLUDE_DIR PCRE2_LIBRARY)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PCRE2 REQUIRED_VARS PCRE2_LIBRARY PCRE2_INCLUDE_DIR)
if(PCRE2_FOUND)
  set(PCRE2_INCLUDE_DIRS "${PCRE2_INCLUDE_DIR}")
  set(PCRE2_LIBRARIES "${PCRE2_LIBRARY}")

  get_filename_component(PCRE2_LIBRARY_EXTNAME "${PCRE2_LIBRARY}" LAST_EXT)
  if (PCRE2_LIBRARY_EXTNAME MATCHES "(.a|.lib)$")
    set(PCRE2_DEFINITIONS PCRE2_STATIC=1)
  endif ()

  find_path(PCRE2POSIX_INCLUDE_DIR
    NAMES pcre2posix.h
    PATHS
      ${PC_PCRE2_INCLUDEDIR}
      ${PC_PCRE2_INCLUDE_DIR}
    )

  find_library(PCRE2POSIX_LIBRARY
    NAMES
      pcre2-posix pcre2-posix-staticd
    PATHS
      ${PC_PCRE2_LIBDIR}
      ${PC_PCRE2_LIBRARIES_DIRS}
    )

  mark_as_advanced(PCRE2POSIX_INCLUDE_DIR AND PCRE2POSIX_LIBRARY)

  if (PCRE2POSIX_INCLUDE_DIR AND PCRE2POSIX_LIBRARY)
    list(APPEND PCRE2_INCLUDE_DIRS "${PCRE2POSIX_INCLUDE_DIR}")
    list(APPEND PCRE2_LIBRARIES "${PCRE2POSIX_LIBRARY}")
  endif ()
endif()

if (PCRE2_FOUND AND NOT TARGET PCRE2::PCRE2)
  add_library(PCRE2::PCRE2 UNKNOWN IMPORTED)
  set_target_properties(PCRE2::PCRE2 PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${PCRE2_DEFINITIONS}")
  set_target_properties(PCRE2::PCRE2 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PCRE2_INCLUDE_DIR}")
  set_target_properties(PCRE2::PCRE2 PROPERTIES IMPORTED_LOCATION "${PCRE2_LIBRARY}")
endif ()
