#
# MIT License
#
# Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
#
# Permission is granted to use, copy, modify, and distribute this software
# under the MIT License. See LICENSE file for details.
#
# LibPostal_FOUND - True if LibPostal was found
# LibPostal_INCLUDE_DIRS - Directories containing LibPostal headers
# LibPostal_LIBRARIES - Libraries to link against LibPostal
# LibPostal_DEFINITIONS - Required compiler definitions for LibPostal
#

if (TARGET LibPostal::LibPostal)
  set(LibPostal_FIND_QUIETLY TRUE)
  set(LibPostal_FOUND TRUE)
  return ()
endif ()

if (LibPostal_INCLUDE_DIR AND LibPostal_LIBRARY)
  set(LibPostal_FIND_QUIETLY TRUE)
endif ()

find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
  pkg_check_modules(PC_LibPostal libpostal)
  set(LibPostal_DEFINITIONS ${PC_LibPostal_CFLAGS_OTHER})
endif ()

find_path(LibPostal_INCLUDE_DIR
  NAMES libpostal/libpostal.h
  PATHS
    ${PC_LibPostal_INCLUDEDIR}
    ${PC_LibPostal_INCLUDE_DIRS}
  PATH_SUFFIXES
    LibPostal
  )
  
find_library(LibPostal_LIBRARY
  NAMES postal
  PATHS
    ${PC_LibPostal_LIBDIR}
    ${PC_LibPostal_LIBRARY_DIRS}
  )

mark_as_advanced(LibPostal_INCLUDE_DIR LibPostal_LIBRARY)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibPostal REQUIRED_VARS LibPostal_LIBRARY LibPostal_INCLUDE_DIR)
if (LIBPOSTAL_FOUND)
  set(LibPostal_FOUND TRUE)
endif ()

if (LibPostal_FOUND)
  set(LibPostal_INCLUDE_DIRS "${LibPostal_INCLUDE_DIR}")
  set(LibPostal_LIBRARIES "${LibPostal_LIBRARY}")
endif ()

if (LibPostal_FOUND AND NOT TARGET LibPostal::LibPostal)
  add_library(LibPostal::LibPostal UNKNOWN IMPORTED)
  set_target_properties(LibPostal::LibPostal PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${LibPostal_DEFINITIONS}")
  set_target_properties(LibPostal::LibPostal PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${LibPostal_INCLUDE_DIR}")
  set_target_properties(LibPostal::LibPostal PROPERTIES IMPORTED_LOCATION "${LibPostal_LIBRARY}")
endif ()
