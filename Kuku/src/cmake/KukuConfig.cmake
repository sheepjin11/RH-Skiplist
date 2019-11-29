# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license.

# Exports target Kuku::kuku
#
# Creates variables:
#   Kuku_FOUND : If Kuku was found
#   KUKU_BUILD_TYPE : The build configuration used
#   KUKU_DEBUG : Set to non-zero value if Kuku is compiled with extra debugging code


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was KukuConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

macro(warning_when_not_quiet msg)
    if(NOT Kuku_FIND_QUIETLY)
        message(WARNING ${msg})
    endif()
endmacro()

macro(status_when_not_quiet msg)
    if(NOT Kuku_FIND_QUIETLY)
        message(STATUS ${msg})
    endif()
endmacro()

set(Kuku_FOUND FALSE)

set(KUKU_BUILD_TYPE Release)
set(KUKU_DEBUG OFF)
set(KUKU_LIB_BUILD_TYPE Static_PIC)

include(${CMAKE_CURRENT_LIST_DIR}/KukuTargets.cmake)

if(TARGET Kuku::kuku)
    status_when_not_quiet("Kuku -> Version ${Kuku_VERSION} detected")
    status_when_not_quiet("Kuku -> Library build type: ${SEAL_LIB_BUILD_TYPE}")
    set(Kuku_FOUND TRUE)
else()
    warning_when_not_quiet("Kuku -> NOT FOUND")
    set(Kuku_FOUND FALSE)
endif()

