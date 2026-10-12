#
# MIT License
#
# Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
#
# Permission is granted to use, copy, modify, and distribute this software
# under the MIT License. See LICENSE file for details.
#

if (TARGET Onion::Onion)
  set(Onion_FIND_QUIETLY TRUE)
  set(Onion_FOUND TRUE)
  return ()
endif ()

if (Onion_INCLUDE_DIR AND Onion_LIBRARY)
  set(Onion_FIND_QUIETLY TRUE)
endif ()

find_path(Onion_INCLUDE_DIR
  NAMES onion/onion.h
  DOC "onion include dir"
  )

find_library(Onion_LIBRARY
  NAMES onion onion_static
  DOC "onion library"
  )

mark_as_advanced(Onion_INCLUDE_DIR Onion_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Onion REQUIRED_VARS Onion_LIBRARY Onion_INCLUDE_DIR)

if (Onion_FOUND)
  set(Onion_DEPENDENCIES "")
  get_filename_component(Onion_LIBRARY_EXTNAME "${Onion_LIBRARY}" LAST_EXT)
  if ("${Onion_LIBRARY_EXTNAME}" MATCHES "(.a|.lib)$")
    find_package(Threads)
    if (Threads_FOUND)
      list(APPEND Onion_DEPENDENCIES Threads::Threads)
    endif ()
  endif ()
  set(Onion_INCLUDE_DIRS ${Onion_INCLUDE_DIR})
  set(Onion_LIBRARIES ${Onion_LIBRARY} ${Onion_DEPENDENCIES})
endif ()

if (Onion_FOUND AND NOT TARGET Onion::Onion)
  add_library(Onion::Onion UNKNOWN IMPORTED)
  set_target_properties(Onion::Onion PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${Onion_INCLUDE_DIR}")
  set_target_properties(Onion::Onion PROPERTIES INTERFACE_LINK_LIBRARIES "${Onion_DEPENDENCIES}")
  set_target_properties(Onion::Onion PROPERTIES IMPORTED_LOCATION "${Onion_LIBRARY}")
endif ()
