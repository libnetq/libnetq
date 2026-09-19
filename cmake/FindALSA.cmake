#
# MIT License
#
# Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
#
# Permission is granted to use, copy, modify, and distribute this software
# under the MIT License. See LICENSE file for details.
#
# ALSA_FOUND - True if ALSA was found
# ALSA_INCLUDE_DIRS - Directories containing ALSA headers
# ALSA_LIBRARIES - Libraries to link against ALSA
# ALSA_DEFINITIONS - Required compiler definitions for ALSA
#

if (TARGET ALSA::ALSA)
  set(ALSA_FIND_QUIETLY TRUE)
  set(ALSA_FOUND TRUE)
  return ()
endif ()

if (ALSA_INCLUDE_DIR AND ALSA_LIBRARY)
  set(ALSA_FIND_QUIETLY TRUE)
endif ()

find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
  pkg_check_modules(PC_ALSA alsa)
  set(ALSA_DEFINITIONS ${PC_ALSA_CFLAGS_OTHER})
endif ()

find_path(ALSA_INCLUDE_DIR
  NAMES alsa/asoundlib.h
  PATHS
    ${PC_ALSA_INCLUDEDIR}
    ${PC_ALSA_INCLUDE_DIRS}
  )
  
find_library(ALSA_LIBRARY
  NAMES asound
  PATHS
    ${PC_ALSA_LIBDIR}
    ${PC_ALSA_LIBRARY_DIRS}
  )

mark_as_advanced(ALSA_INCLUDE_DIR ALSA_LIBRARY)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ALSA REQUIRED_VARS ALSA_LIBRARY ALSA_INCLUDE_DIR)

if (ALSA_FOUND)
  set(ALSA_INCLUDE_DIRS "${ALSA_INCLUDE_DIR}")
  set(ALSA_LIBRARIES "${ALSA_LIBRARY}")
endif ()

if (ALSA_FOUND AND NOT TARGET ALSA::ALSA)
  add_library(ALSA::ALSA UNKNOWN IMPORTED)
  set_target_properties(ALSA::ALSA PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${ALSA_DEFINITIONS}")
  set_target_properties(ALSA::ALSA PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${ALSA_INCLUDE_DIR}")
  set_target_properties(ALSA::ALSA PROPERTIES IMPORTED_LOCATION "${ALSA_LIBRARY}")
endif ()
