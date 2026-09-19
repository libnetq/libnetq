#
# MIT License
#
# Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
#
# Permission is granted to use, copy, modify, and distribute this software
# under the MIT License. See LICENSE file for details.
#
# ICAL_FOUND - True if ICAL was found
# ICAL_INCLUDE_DIRS - Directories containing ICAL headers
# ICAL_LIBRARIES - Libraries to link against ICAL
# ICAL_DEFINITIONS - Required compiler definitions for ICAL
#

if (TARGET ICAL::ICAL)
  set(ICAL_FIND_QUIETLY TRUE)
  set(ICAL_FOUND TRUE)
  return ()
endif ()

if (ICAL_INCLUDE_DIR AND ICAL_LIBRARY)
  set(ICAL_FIND_QUIETLY TRUE)
endif ()

find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
  pkg_check_modules(PC_ICAL libical)
  set(ICAL_DEFINITIONS ${PC_ICAL_CFLAGS_OTHER})
endif ()

find_path(ICAL_INCLUDE_DIR
  NAMES libical/ical.h
  PATHS
    ${PC_ICAL_INCLUDEDIR}
    ${PC_ICAL_INCLUDE_DIRS}
  )
  
find_library(ICAL_LIBRARY
  NAMES ical
  PATHS
    ${PC_ICAL_LIBDIR}
    ${PC_ICAL_LIBRARY_DIRS}
  )

mark_as_advanced(ICAL_INCLUDE_DIR ICAL_LIBRARY)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ICAL REQUIRED_VARS ICAL_LIBRARY ICAL_INCLUDE_DIR)

if (ICAL_FOUND)
  set(ICAL_INCLUDE_DIRS "${ICAL_INCLUDE_DIR}")
  set(ICAL_LIBRARIES "${ICAL_LIBRARY}")
endif ()

if (ICAL_FOUND AND NOT TARGET ICAL::ICAL)
  add_library(ICAL::ICAL UNKNOWN IMPORTED)
  set_target_properties(ICAL::ICAL PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${ICAL_DEFINITIONS}")
  set_target_properties(ICAL::ICAL PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${ICAL_INCLUDE_DIR}")
  set_target_properties(ICAL::ICAL PROPERTIES IMPORTED_LOCATION "${ICAL_LIBRARY}")
endif ()
