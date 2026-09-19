#
# MIT License
#
# Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
#
# Permission is granted to use, copy, modify, and distribute this software
# under the MIT License. See LICENSE file for details.
#

if (TARGET Sodium::Sodium)
  set(Sodium_FIND_QUIETLY TRUE)
  set(Sodium_FOUND TRUE)
  return ()
endif ()

if (Sodium_LIBRARIES AND Sodium_INCLUDE_DIRS)
  set(Sodium_FIND_QUIETLY TRUE)
endif ()

find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
  pkg_check_modules(PC_Sodium libsodium)
  set(Sodium_DEFINITIONS ${PC_Sodium_CFLAGS_OTHER})
endif ()

find_path(Sodium_INCLUDE_DIR
  NAMES
    sodium.h
  PATHS
    ${PC_Sodium_INCLUDEDIR}
    ${PC_Sodium_INCLUDE_DIRS}
  )

find_library(Sodium_LIBRARY
  NAMES
    sodium libsodium
  PATHS
    ${PC_Sodium_LIBDIR}
    ${PC_Sodium_LIBRARY_DIRS}
  )

get_filename_component(Sodium_LIBRARY_EXTNAME "${Sodium_LIBRARY}" LAST_EXT)
if ("${Sodium_LIBRARY_EXTNAME}" MATCHES "(.a|.lib)$")
  list(APPEND Sodium_DEFINITIONS SODIUM_STATIC)
endif ()

mark_as_advanced(Sodium_INCLUDE_DIR Sodium_LIBRARY)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Sodium REQUIRED_VARS Sodium_LIBRARY Sodium_INCLUDE_DIR)
if (SODIUM_FOUND)
  set(Sodium_FOUND TRUE)
endif ()

if (Sodium_FOUND)
  set(Sodium_INCLUDE_DIRS "${Sodium_INCLUDE_DIR}")
  set(Sodium_LIBRARIES "${Sodium_LIBRARY}")
endif ()

if (Sodium_FOUND AND NOT TARGET Sodium::Sodium)
  add_library(Sodium::Sodium UNKNOWN IMPORTED)
  set_target_properties(Sodium::Sodium PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${Sodium_INCLUDE_DIR}")
  set_target_properties(Sodium::Sodium PROPERTIES IMPORTED_LOCATION "${Sodium_LIBRARY}")
  set_target_properties(Sodium::Sodium PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${Sodium_DEFINITIONS}")
endif ()
