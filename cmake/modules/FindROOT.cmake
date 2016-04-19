# - Finds ROOT instalation. Copied from https://root.cern.ch/
# This module sets up ROOT information
# It defines:
# ROOT_FOUND          If the ROOT is found
# ROOT_INCLUDE_DIR    PATH to the include directory
# ROOT_INCLUDE_DIRS   PATH to the include directories (not cached)
# ROOT_LIBRARIES      Most common libraries
# ROOT_<name>_LIBRARY Full path to the library <name>
# ROOT_LIBRARY_DIR    PATH to the library directory
# ROOT_DEFINITIONS    Compiler definitions
# ROOT_CXX_FLAGS      Compiler flags to used by client packages
# ROOT_C_FLAGS        Compiler flags to used by client packages
#
# Updated by K. Smith (ksmith37@nd.edu) to properly handle
#  dependencies in ROOT_GENERATE_DICTIONARY

find_program(ROOT_CONFIG_EXECUTABLE root-config
  HINTS $ENV{ROOTSYS}/bin)

execute_process(
    COMMAND ${ROOT_CONFIG_EXECUTABLE} --prefix
    OUTPUT_VARIABLE ROOTSYS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
    COMMAND ${ROOT_CONFIG_EXECUTABLE} --version
    OUTPUT_VARIABLE ROOT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
    COMMAND ${ROOT_CONFIG_EXECUTABLE} --incdir
    OUTPUT_VARIABLE ROOT_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
set(ROOT_INCLUDE_DIRS ${ROOT_INCLUDE_DIR})

execute_process(
    COMMAND ${ROOT_CONFIG_EXECUTABLE} --libdir
    OUTPUT_VARIABLE ROOT_LIBRARY_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
set(ROOT_LIBRARY_DIRS ${ROOT_LIBRARY_DIR})

#set(rootlibs Core Cint RIO Net Hist Graf Graf3d Gpad Tree Rint Postscript Matrix Physics MathCore Thread)
set(ROOT_LIBRARIES)
foreach(_cpt ${rootlibs} ${ROOT_FIND_COMPONENTS})
  find_library(ROOT_${_cpt}_LIBRARY ${_cpt} HINTS ${ROOT_LIBRARY_DIR})
  if(ROOT_${_cpt}_LIBRARY)
    mark_as_advanced(ROOT_${_cpt}_LIBRARY)
    list(APPEND ROOT_LIBRARIES ${ROOT_${_cpt}_LIBRARY})
    list(REMOVE_ITEM ROOT_FIND_COMPONENTS ${_cpt})
  elseif(ROOT_FIND_REQUIRED_${_cpt})
    message(FATAL_ERROR "Unable to find the requested ROOT library '${_cpt}'.")
  endif()
endforeach()
list(REMOVE_DUPLICATES ROOT_LIBRARIES)
