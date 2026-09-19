#
# MIT License
#
# Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
#
# Permission is granted to use, copy, modify, and distribute this software
# under the MIT License. See LICENSE file for details.
#
# GeoIP_FOUND - True if GeoIP was found
# GeoIP_INCLUDE_DIRS - Directories containing GeoIP headers
# GeoIP_LIBRARIES - Libraries to link against GeoIP
# GeoIP_DEFINITIONS - Required compiler definitions for GeoIP
#

if (TARGET GeoIP::GeoIP)
  set(GeoIP_FIND_QUIETLY TRUE)
  set(GeoIP_FOUND TRUE)
  return ()
endif ()

if (GeoIP_INCLUDE_DIR AND GeoIP_LIBRARY)
  set(GeoIP_FIND_QUIETLY TRUE)
endif ()

find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
  pkg_check_modules(PC_GeoIP geoip)
  set(GeoIP_DEFINITIONS ${PC_GeoIP_CFLAGS_OTHER})
endif ()

find_path(GeoIP_INCLUDE_DIR
  NAMES GeoIP.h
  PATHS
    ${PC_GeoIP_INCLUDEDIR}
    ${PC_GeoIP_INCLUDE_DIRS}
  PATH_SUFFIXES
    GeoIP
  )
  
find_library(GeoIP_LIBRARY
  NAMES GeoIP
  PATHS
    ${PC_GeoIP_LIBDIR}
    ${PC_GeoIP_LIBRARY_DIRS}
  )

mark_as_advanced(GeoIP_INCLUDE_DIR GeoIP_LIBRARY)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GeoIP REQUIRED_VARS GeoIP_LIBRARY GeoIP_INCLUDE_DIR)
if (GEOIP_FOUND)
  set(GeoIP_FOUND TRUE)
endif ()

if (GeoIP_FOUND)
  set(GeoIP_INCLUDE_DIRS "${GeoIP_INCLUDE_DIR}")
  set(GeoIP_LIBRARIES "${GeoIP_LIBRARY}")
endif ()

if (GeoIP_FOUND AND NOT TARGET GeoIP::GeoIP)
  add_library(GeoIP::GeoIP UNKNOWN IMPORTED)
  set_target_properties(GeoIP::GeoIP PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${GeoIP_DEFINITIONS}")
  set_target_properties(GeoIP::GeoIP PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${GeoIP_INCLUDE_DIR}")
  set_target_properties(GeoIP::GeoIP PROPERTIES IMPORTED_LOCATION "${GeoIP_LIBRARY}")
endif ()
