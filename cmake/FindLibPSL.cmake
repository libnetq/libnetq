#
# MIT License
#
# Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
#
# Permission is granted to use, copy, modify, and distribute this software
# under the MIT License. See LICENSE file for details.
#
# LibPSL_FOUND - True if LibPSL was found
# LibPSL_INCLUDE_DIRS - Directories containing LibPSL headers
# LibPSL_LIBRARIES - Libraries to link against LibPSL
# LibPSL_DEFINITIONS - Required compiler definitions for LibPSL
#

if (TARGET LibPSL::LibPSL)
  set(LibPSL_FIND_QUIETLY TRUE)
  set(LibPSL_FOUND TRUE)
  return ()
endif ()

if (LibPSL_INCLUDE_DIR AND LibPSL_LIBRARY)
  set(LibPSL_FIND_QUIETLY TRUE)
endif ()

find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
  pkg_check_modules(PC_LibPSL libpsl)
  set(LibPSL_DEFINITIONS ${PC_LibPSL_CFLAGS_OTHER})
endif ()

find_path(LibPSL_INCLUDE_DIR
  NAMES libpsl.h
  PATHS
    ${PC_LibPSL_INCLUDEDIR}
    ${PC_LibPSL_INCLUDE_DIRS}
  PATH_SUFFIXES
    LibPSL
  )
  
find_library(LibPSL_LIBRARY
  NAMES psl
  PATHS
    ${PC_LibPSL_LIBDIR}
    ${PC_LibPSL_LIBRARY_DIRS}
  )

mark_as_advanced(LibPSL_INCLUDE_DIR LibPSL_LIBRARY)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibPSL REQUIRED_VARS LibPSL_LIBRARY LibPSL_INCLUDE_DIR)
if (LibPSL_FOUND)
  set(LibPSL_FOUND TRUE)
endif ()

if (LibPSL_FOUND)
  set(LibPSL_INCLUDE_DIRS "${LibPSL_INCLUDE_DIR}")
  set(LibPSL_LIBRARIES "${LibPSL_LIBRARY}")
endif ()

if (LibPSL_FOUND AND NOT TARGET LibPSL::LibPSL)
  add_library(LibPSL::LibPSL UNKNOWN IMPORTED)
  set_target_properties(LibPSL::LibPSL PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${LibPSL_DEFINITIONS}")
  set_target_properties(LibPSL::LibPSL PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${LibPSL_INCLUDE_DIR}")
  set_target_properties(LibPSL::LibPSL PROPERTIES IMPORTED_LOCATION "${LibPSL_LIBRARY}")
endif ()
