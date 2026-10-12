#
# MIT License
#
# Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
#
# Permission is granted to use, copy, modify, and distribute this software
# under the MIT License. See LICENSE file for details.
#
# Blend2D_FOUND - True if Blend2D was found
# Blend2D_INCLUDE_DIRS - Directories containing Blend2D headers
# Blend2D_LIBRARIES - Libraries to link against Blend2D
# Blend2D_DEFINITIONS - Required compiler definitions for Blend2D
# Blend2D_VERSION - Version of Blend2D found
#
# Blend2D::Blend2D - Imported target
#
# Blend2D's own CMake package (blend2d::blend2d) is preferred, as it carries
# BL_STATIC for a static build and the library's link needs. It installs no
# version file, so the version always comes from blend2d/core/api.h.
#

if (TARGET Blend2D::Blend2D)
  set(Blend2D_FIND_QUIETLY TRUE)
  set(Blend2D_FOUND TRUE)
  return ()
endif ()

if (Blend2D_INCLUDE_DIR AND Blend2D_LIBRARY)
  set(Blend2D_FIND_QUIETLY TRUE)
endif ()

find_package(blend2d CONFIG QUIET)

if (TARGET blend2d::blend2d)
  get_target_property(Blend2D_INCLUDE_DIR blend2d::blend2d INTERFACE_INCLUDE_DIRECTORIES)
  set(Blend2D_LIBRARY blend2d::blend2d)
else ()
  find_package(PkgConfig QUIET)
  if (PkgConfig_FOUND)
    pkg_check_modules(PC_Blend2D QUIET blend2d)
    set(Blend2D_DEFINITIONS ${PC_Blend2D_CFLAGS_OTHER})
  endif ()

  find_path(Blend2D_INCLUDE_DIR
    NAMES blend2d/blend2d.h
    PATHS
      ${PC_Blend2D_INCLUDEDIR}
      ${PC_Blend2D_INCLUDE_DIRS}
    )

  find_library(Blend2D_LIBRARY
    NAMES blend2d
    PATHS
      ${PC_Blend2D_LIBDIR}
      ${PC_Blend2D_LIBRARY_DIRS}
    )

  # without it blend2d.h declares the functions __declspec(dllimport)
  if ("${Blend2D_LIBRARY}" MATCHES "\\.(a|lib)$" AND NOT "${Blend2D_LIBRARY}" MATCHES "\\.dll\\.a$")
    list(APPEND Blend2D_DEFINITIONS BL_STATIC)
  endif ()

  # a static Blend2D needs threads, and librt for its shared memory on Linux
  set(Blend2D_DEPENDENCIES "")
  find_package(Threads QUIET)
  if (TARGET Threads::Threads)
    list(APPEND Blend2D_DEPENDENCIES Threads::Threads)
  endif ()
  if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_library(Blend2D_RT_LIBRARY rt)
    if (Blend2D_RT_LIBRARY)
      list(APPEND Blend2D_DEPENDENCIES "${Blend2D_RT_LIBRARY}")
    endif ()
  endif ()

  mark_as_advanced(Blend2D_INCLUDE_DIR Blend2D_LIBRARY Blend2D_RT_LIBRARY)
endif ()

foreach (_dir IN LISTS Blend2D_INCLUDE_DIR)
  if (NOT Blend2D_VERSION AND EXISTS "${_dir}/blend2d/core/api.h")
    file(STRINGS "${_dir}/blend2d/core/api.h" _version_line
      REGEX "^#define +BL_VERSION +BL_MAKE_VERSION\\(")
    if (_version_line MATCHES "BL_MAKE_VERSION\\( *([0-9]+) *, *([0-9]+) *, *([0-9]+) *\\)")
      set(Blend2D_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
    endif ()
    unset(_version_line)
  endif ()
endforeach ()
unset(_dir)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Blend2D REQUIRED_VARS Blend2D_LIBRARY Blend2D_INCLUDE_DIR VERSION_VAR Blend2D_VERSION)

if (Blend2D_FOUND)
  set(Blend2D_INCLUDE_DIRS "${Blend2D_INCLUDE_DIR}")
  set(Blend2D_LIBRARIES "${Blend2D_LIBRARY}" ${Blend2D_DEPENDENCIES})
endif ()

if (Blend2D_FOUND AND NOT TARGET Blend2D::Blend2D)
  if (TARGET blend2d::blend2d)
    add_library(Blend2D::Blend2D INTERFACE IMPORTED)
    set_target_properties(Blend2D::Blend2D PROPERTIES INTERFACE_LINK_LIBRARIES blend2d::blend2d)
  else ()
    add_library(Blend2D::Blend2D UNKNOWN IMPORTED)
    set_target_properties(Blend2D::Blend2D PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${Blend2D_INCLUDE_DIR}")
    set_target_properties(Blend2D::Blend2D PROPERTIES IMPORTED_LOCATION "${Blend2D_LIBRARY}")
    set_target_properties(Blend2D::Blend2D PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${Blend2D_DEFINITIONS}")
    set_target_properties(Blend2D::Blend2D PROPERTIES INTERFACE_LINK_LIBRARIES "${Blend2D_DEPENDENCIES}")
  endif ()
endif ()
