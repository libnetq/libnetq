#
# MIT License
#
# Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
#
# Permission is granted to use, copy, modify, and distribute this software
# under the MIT License. See LICENSE file for details.
#
# Skia_FOUND - True if Skia was found
# Skia_INCLUDE_DIRS - Directories containing Skia headers
# Skia_LIBRARIES - Libraries to link against Skia
# Skia_DEFINITIONS - Required compiler definitions for Skia
# Skia_VERSION - Skia milestone found (e.g. 155)
#
# Skia::Skia - Imported target
#

if (TARGET Skia::Skia)
  set(Skia_FIND_QUIETLY TRUE)
  set(Skia_FOUND TRUE)
  return ()
endif ()

if (Skia_INCLUDE_DIR AND Skia_LIBRARY)
  set(Skia_FIND_QUIETLY TRUE)
endif ()

find_package(skia CONFIG QUIET)

if (TARGET skia::skia)
  get_target_property(Skia_INCLUDE_DIR skia::skia INTERFACE_INCLUDE_DIRECTORIES)
  get_target_property(Skia_DEFINITIONS skia::skia INTERFACE_COMPILE_DEFINITIONS)
  set(Skia_LIBRARY skia::skia)
  set(Skia_VERSION "${skia_VERSION}")
else ()
  find_package(PkgConfig QUIET)
  if (PkgConfig_FOUND)
    pkg_check_modules(PC_Skia QUIET skia)
    set(Skia_DEFINITIONS ${PC_Skia_CFLAGS_OTHER})
  endif ()

  find_path(Skia_INCLUDE_DIR
    NAMES include/core/SkCanvas.h
    PATHS
      ${PC_Skia_INCLUDEDIR}
      ${PC_Skia_INCLUDE_DIRS}
    PATH_SUFFIXES
      skia
    )

  find_library(Skia_LIBRARY
    NAMES skia
    PATHS
      ${PC_Skia_LIBDIR}
      ${PC_Skia_LIBRARY_DIRS}
    )

  mark_as_advanced(Skia_INCLUDE_DIR Skia_LIBRARY)

  if (Skia_INCLUDE_DIR)
    file(STRINGS "${Skia_INCLUDE_DIR}/include/core/SkMilestone.h" Skia_VERSION
      REGEX "^#define +SK_MILESTONE +[0-9]+")
    string(REGEX MATCH "[0-9]+$" Skia_VERSION "${Skia_VERSION}")
  endif ()
endif ()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Skia REQUIRED_VARS Skia_LIBRARY Skia_INCLUDE_DIR VERSION_VAR Skia_VERSION)

if (Skia_FOUND)
  set(Skia_INCLUDE_DIRS "${Skia_INCLUDE_DIR}")
  set(Skia_LIBRARIES "${Skia_LIBRARY}")
endif ()

if (Skia_FOUND AND NOT TARGET Skia::Skia)
  if (TARGET skia::skia)
    add_library(Skia::Skia INTERFACE IMPORTED)
    set_target_properties(Skia::Skia PROPERTIES INTERFACE_LINK_LIBRARIES skia::skia)
  else ()
    add_library(Skia::Skia UNKNOWN IMPORTED)
    set_target_properties(Skia::Skia PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${Skia_INCLUDE_DIR}")
    set_target_properties(Skia::Skia PROPERTIES IMPORTED_LOCATION "${Skia_LIBRARY}")
    set_target_properties(Skia::Skia PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES CXX)
    set_target_properties(Skia::Skia PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${Skia_DEFINITIONS}")
    # Skia's headers need C++20, the standard it is built with
    set_target_properties(Skia::Skia PROPERTIES INTERFACE_COMPILE_FEATURES cxx_std_20)
    if (PC_Skia_FOUND)
      set_target_properties(Skia::Skia PROPERTIES INTERFACE_LINK_LIBRARIES "${PC_Skia_STATIC_LDFLAGS}")
    endif ()
  endif ()
endif ()
