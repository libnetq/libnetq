#
# MIT License
#
# Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
#
# Permission is granted to use, copy, modify, and distribute this software
# under the MIT License. See LICENSE file for details.
#
# NanoVG_FOUND - True if NanoVG was found
# NanoVG_INCLUDE_DIRS - Directories containing NanoVG headers
# NanoVG_LIBRARIES - Libraries to link against NanoVG
# NanoVG_DEFINITIONS - Required compiler definitions for NanoVG
#
# NanoVG::NanoVG - Imported target
#
# Upstream ships no build system of its own, so NanoVG is either installed
# with a CMake package (nanovg::nanovg) or as a bare nanovg.h + libnanovg.
# The package is preferred, as it carries the library's own link needs.
#

if (TARGET NanoVG::NanoVG)
  set(NanoVG_FIND_QUIETLY TRUE)
  set(NanoVG_FOUND TRUE)
  return ()
endif ()

if (NanoVG_INCLUDE_DIR AND NanoVG_LIBRARY)
  set(NanoVG_FIND_QUIETLY TRUE)
endif ()

find_package(nanovg CONFIG QUIET)

if (TARGET nanovg::nanovg)
  get_target_property(NanoVG_INCLUDE_DIR nanovg::nanovg INTERFACE_INCLUDE_DIRECTORIES)
  set(NanoVG_LIBRARY nanovg::nanovg)
else ()
  find_package(PkgConfig QUIET)
  if (PkgConfig_FOUND)
    pkg_check_modules(PC_NanoVG QUIET nanovg)
    set(NanoVG_DEFINITIONS ${PC_NanoVG_CFLAGS_OTHER})
  endif ()

  find_path(NanoVG_INCLUDE_DIR
    NAMES nanovg.h
    PATHS
      ${PC_NanoVG_INCLUDEDIR}
      ${PC_NanoVG_INCLUDE_DIRS}
    PATH_SUFFIXES
      nanovg
    )

  find_library(NanoVG_LIBRARY
    NAMES nanovg
    PATHS
      ${PC_NanoVG_LIBDIR}
      ${PC_NanoVG_LIBRARY_DIRS}
    )

  # nanovg.c uses the C math library
  set(NanoVG_DEPENDENCIES "")
  find_library(NanoVG_M_LIBRARY m)
  if (NanoVG_M_LIBRARY)
    list(APPEND NanoVG_DEPENDENCIES "${NanoVG_M_LIBRARY}")
  endif ()

  mark_as_advanced(NanoVG_INCLUDE_DIR NanoVG_LIBRARY NanoVG_M_LIBRARY)
endif ()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NanoVG REQUIRED_VARS NanoVG_LIBRARY NanoVG_INCLUDE_DIR)

if (NanoVG_FOUND)
  set(NanoVG_INCLUDE_DIRS "${NanoVG_INCLUDE_DIR}")
  set(NanoVG_LIBRARIES "${NanoVG_LIBRARY}" ${NanoVG_DEPENDENCIES})
endif ()

if (NanoVG_FOUND AND NOT TARGET NanoVG::NanoVG)
  if (TARGET nanovg::nanovg)
    add_library(NanoVG::NanoVG INTERFACE IMPORTED)
    set_target_properties(NanoVG::NanoVG PROPERTIES INTERFACE_LINK_LIBRARIES nanovg::nanovg)
  else ()
    add_library(NanoVG::NanoVG UNKNOWN IMPORTED)
    set_target_properties(NanoVG::NanoVG PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${NanoVG_INCLUDE_DIR}")
    set_target_properties(NanoVG::NanoVG PROPERTIES IMPORTED_LOCATION "${NanoVG_LIBRARY}")
    set_target_properties(NanoVG::NanoVG PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${NanoVG_DEFINITIONS}")
    set_target_properties(NanoVG::NanoVG PROPERTIES INTERFACE_LINK_LIBRARIES "${NanoVG_DEPENDENCIES}")
  endif ()
endif ()
