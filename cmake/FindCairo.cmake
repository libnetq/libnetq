#
# MIT License
#
# Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
#
# Permission is granted to use, copy, modify, and distribute this software
# under the MIT License. See LICENSE file for details.
#
# Cairo_FOUND - True if libcairo was found
# Cairo_INCLUDE_DIRS - Directories containing libcairo headers
# Cairo_LIBRARIES - Libraries to link against libcairo
# Cairo_DEFINITIONS - Required compiler definitions for libcairo
#
# Cairo::Cairo - Imported target
#
# COMPONENTS name cairo's surface backends (xlib, xcb, quartz, win32, ft,
# ...): each needs its cairo-<name>.h header, and its cairo-<name> pkg-config
# module joins the query. A static libcairo gets its private dependencies
# (pixman, libpng, the backends' libraries, ...) from pkg-config --static.
#

if (TARGET Cairo::Cairo)
  set(Cairo_FIND_QUIETLY TRUE)
  set(Cairo_FOUND TRUE)
  return ()
endif ()

if (Cairo_INCLUDE_DIR AND Cairo_LIBRARY)
  set(Cairo_FIND_QUIETLY TRUE)
endif ()

set(Cairo_PC_MODULES cairo)
foreach (_component IN LISTS Cairo_FIND_COMPONENTS)
  list(APPEND Cairo_PC_MODULES cairo-${_component})
endforeach ()

find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
  set(_Cairo_destdir "$ENV{DESTDIR}")
  unset(ENV{DESTDIR})
  pkg_check_modules(PC_Cairo ${Cairo_PC_MODULES})
  set(ENV{DESTDIR} "${_Cairo_destdir}")
  unset(_Cairo_destdir)
  set(Cairo_DEFINITIONS ${PC_Cairo_CFLAGS_OTHER})
endif ()

find_path(Cairo_INCLUDE_DIR
  NAMES cairo.h
  PATHS
    ${PC_Cairo_INCLUDEDIR}
    ${PC_Cairo_INCLUDE_DIRS}
  PATH_SUFFIXES
    cairo
  )

find_library(Cairo_LIBRARY
  NAMES cairo
  PATHS
    ${PC_Cairo_LIBDIR}
    ${PC_Cairo_LIBRARY_DIRS}
  )

mark_as_advanced(Cairo_INCLUDE_DIR Cairo_LIBRARY)

foreach (_component IN LISTS Cairo_FIND_COMPONENTS)
  set(Cairo_${_component}_FOUND FALSE)
  if (Cairo_INCLUDE_DIR AND EXISTS "${Cairo_INCLUDE_DIR}/cairo-${_component}.h")
    set(Cairo_${_component}_FOUND TRUE)
  endif ()
endforeach ()

set(Cairo_DEPENDENCIES "")
if ("${Cairo_LIBRARY}" MATCHES "\\.(a|lib)$" AND NOT "${Cairo_LIBRARY}" MATCHES "\\.dll\\.a$")
  if (PC_Cairo_FOUND)
    set(Cairo_DEFINITIONS "")
    foreach (_flag IN LISTS PC_Cairo_STATIC_CFLAGS_OTHER)
      if (_flag MATCHES "^-D(.+)$")
        list(APPEND Cairo_DEFINITIONS "${CMAKE_MATCH_1}")
      endif ()
    endforeach ()
    set(Cairo_DEPENDENCIES ${PC_Cairo_STATIC_LDFLAGS})
    # pkg-config reports "-framework Foo" as two list items, which CMake would
    # turn into "-framework -lFoo"; keep each pair together as one linker flag.
    string(REPLACE "-framework;" "-Wl,-framework," Cairo_DEPENDENCIES "${Cairo_DEPENDENCIES}")
  endif ()
  # without it cairo.h declares the functions __declspec(dllimport)
  if (WIN32)
    list(APPEND Cairo_DEFINITIONS CAIRO_WIN32_STATIC_BUILD)
    list(REMOVE_DUPLICATES Cairo_DEFINITIONS)
  endif ()
endif ()

if (Cairo_INCLUDE_DIR)
  file(READ "${Cairo_INCLUDE_DIR}/cairo-version.h" Cairo_VERSION_CONTENT)

  string(REGEX MATCH "#define +CAIRO_VERSION_MAJOR +([0-9]+)" _dummy "${Cairo_VERSION_CONTENT}")
  set(Cairo_VERSION_MAJOR "${CMAKE_MATCH_1}")

  string(REGEX MATCH "#define +CAIRO_VERSION_MINOR +([0-9]+)" _dummy "${Cairo_VERSION_CONTENT}")
  set(Cairo_VERSION_MINOR "${CMAKE_MATCH_1}")

  string(REGEX MATCH "#define +CAIRO_VERSION_MICRO +([0-9]+)" _dummy "${Cairo_VERSION_CONTENT}")
  set(Cairo_VERSION_PATCH "${CMAKE_MATCH_1}")

  set(Cairo_VERSION "${Cairo_VERSION_MAJOR}.${Cairo_VERSION_MINOR}.${Cairo_VERSION_PATCH}")
  unset(Cairo_VERSION_CONTENT)
endif ()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Cairo REQUIRED_VARS Cairo_LIBRARY Cairo_INCLUDE_DIR VERSION_VAR Cairo_VERSION HANDLE_COMPONENTS)
if (CAIRO_FOUND)
  set(Cairo_FOUND TRUE)
endif ()

if (Cairo_FOUND)
  set(Cairo_INCLUDE_DIRS ${Cairo_INCLUDE_DIR} ${PC_Cairo_STATIC_INCLUDE_DIRS})
  list(REMOVE_DUPLICATES Cairo_INCLUDE_DIRS)
  set(Cairo_LIBRARIES ${Cairo_LIBRARY} ${Cairo_DEPENDENCIES})
endif ()

if (Cairo_FOUND AND NOT TARGET Cairo::Cairo)
  add_library(Cairo::Cairo UNKNOWN IMPORTED)
  set_target_properties(Cairo::Cairo PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${Cairo_INCLUDE_DIRS}")
  set_target_properties(Cairo::Cairo PROPERTIES IMPORTED_LOCATION "${Cairo_LIBRARY}")
  set_target_properties(Cairo::Cairo PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${Cairo_DEFINITIONS}")
  set_target_properties(Cairo::Cairo PROPERTIES INTERFACE_LINK_LIBRARIES "${Cairo_DEPENDENCIES}")
endif ()
