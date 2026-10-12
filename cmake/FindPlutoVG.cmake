#
# MIT License
#
# Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
#
# Permission is granted to use, copy, modify, and distribute this software
# under the MIT License. See LICENSE file for details.
#
# PlutoVG_FOUND - True if PlutoVG was found
# PlutoVG_INCLUDE_DIRS - Directories containing PlutoVG headers
# PlutoVG_LIBRARIES - Libraries to link against PlutoVG
# PlutoVG_DEFINITIONS - Required compiler definitions for PlutoVG
# PlutoVG_VERSION - Version of PlutoVG found
#
# PlutoVG::PlutoVG - Imported target
#
# PlutoVG's own CMake package (plutovg::plutovg) is preferred, as it carries
# PLUTOVG_BUILD_STATIC for a static build and the library's link needs.
#

if (TARGET PlutoVG::PlutoVG)
  set(PlutoVG_FIND_QUIETLY TRUE)
  set(PlutoVG_FOUND TRUE)
  return ()
endif ()

if (PlutoVG_INCLUDE_DIR AND PlutoVG_LIBRARY)
  set(PlutoVG_FIND_QUIETLY TRUE)
endif ()

find_package(plutovg CONFIG QUIET)

if (TARGET plutovg::plutovg)
  get_target_property(PlutoVG_INCLUDE_DIR plutovg::plutovg INTERFACE_INCLUDE_DIRECTORIES)
  set(PlutoVG_LIBRARY plutovg::plutovg)
  set(PlutoVG_VERSION "${plutovg_VERSION}")
else ()
  find_package(PkgConfig QUIET)
  if (PkgConfig_FOUND)
    pkg_check_modules(PC_PlutoVG QUIET plutovg)
    set(PlutoVG_DEFINITIONS ${PC_PlutoVG_CFLAGS_OTHER})
  endif ()

  find_path(PlutoVG_INCLUDE_DIR
    NAMES plutovg.h
    PATHS
      ${PC_PlutoVG_INCLUDEDIR}
      ${PC_PlutoVG_INCLUDE_DIRS}
    PATH_SUFFIXES
      plutovg
    )

  find_library(PlutoVG_LIBRARY
    NAMES plutovg
    PATHS
      ${PC_PlutoVG_LIBDIR}
      ${PC_PlutoVG_LIBRARY_DIRS}
    )

  # without it plutovg.h declares the functions __declspec(dllimport)
  if ("${PlutoVG_LIBRARY}" MATCHES "\\.(a|lib)$" AND NOT "${PlutoVG_LIBRARY}" MATCHES "\\.dll\\.a$")
    list(APPEND PlutoVG_DEFINITIONS PLUTOVG_BUILD_STATIC)
  endif ()

  # plutovg.c uses the C math library and threads
  set(PlutoVG_DEPENDENCIES "")
  find_library(PlutoVG_M_LIBRARY m)
  if (PlutoVG_M_LIBRARY)
    list(APPEND PlutoVG_DEPENDENCIES "${PlutoVG_M_LIBRARY}")
  endif ()
  find_package(Threads QUIET)
  if (TARGET Threads::Threads)
    list(APPEND PlutoVG_DEPENDENCIES Threads::Threads)
  endif ()

  mark_as_advanced(PlutoVG_INCLUDE_DIR PlutoVG_LIBRARY PlutoVG_M_LIBRARY)

  if (PlutoVG_INCLUDE_DIR)
    file(READ "${PlutoVG_INCLUDE_DIR}/plutovg.h" PlutoVG_VERSION_CONTENT)

    string(REGEX MATCH "#define +PLUTOVG_VERSION_MAJOR +([0-9]+)" _dummy "${PlutoVG_VERSION_CONTENT}")
    set(PlutoVG_VERSION_MAJOR "${CMAKE_MATCH_1}")

    string(REGEX MATCH "#define +PLUTOVG_VERSION_MINOR +([0-9]+)" _dummy "${PlutoVG_VERSION_CONTENT}")
    set(PlutoVG_VERSION_MINOR "${CMAKE_MATCH_1}")

    string(REGEX MATCH "#define +PLUTOVG_VERSION_MICRO +([0-9]+)" _dummy "${PlutoVG_VERSION_CONTENT}")
    set(PlutoVG_VERSION_PATCH "${CMAKE_MATCH_1}")

    set(PlutoVG_VERSION "${PlutoVG_VERSION_MAJOR}.${PlutoVG_VERSION_MINOR}.${PlutoVG_VERSION_PATCH}")
    unset(PlutoVG_VERSION_CONTENT)
  endif ()
endif ()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PlutoVG REQUIRED_VARS PlutoVG_LIBRARY PlutoVG_INCLUDE_DIR VERSION_VAR PlutoVG_VERSION)

if (PlutoVG_FOUND)
  set(PlutoVG_INCLUDE_DIRS "${PlutoVG_INCLUDE_DIR}")
  set(PlutoVG_LIBRARIES "${PlutoVG_LIBRARY}" ${PlutoVG_DEPENDENCIES})
endif ()

if (PlutoVG_FOUND AND NOT TARGET PlutoVG::PlutoVG)
  if (TARGET plutovg::plutovg)
    add_library(PlutoVG::PlutoVG INTERFACE IMPORTED)
    set_target_properties(PlutoVG::PlutoVG PROPERTIES INTERFACE_LINK_LIBRARIES plutovg::plutovg)
  else ()
    add_library(PlutoVG::PlutoVG UNKNOWN IMPORTED)
    set_target_properties(PlutoVG::PlutoVG PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PlutoVG_INCLUDE_DIR}")
    set_target_properties(PlutoVG::PlutoVG PROPERTIES IMPORTED_LOCATION "${PlutoVG_LIBRARY}")
    set_target_properties(PlutoVG::PlutoVG PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${PlutoVG_DEFINITIONS}")
    set_target_properties(PlutoVG::PlutoVG PROPERTIES INTERFACE_LINK_LIBRARIES "${PlutoVG_DEPENDENCIES}")
  endif ()
endif ()
