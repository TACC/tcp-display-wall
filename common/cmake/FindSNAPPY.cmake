#/* =======================================================================================
#   This file is released as part of TCP Display Wall module for TCP Bridged
#   Display Wall module for OSPray
#
#   https://github.com/TACC/tcp-display-wall
#
#   Copyright 2017-2018 Texas Advanced Computing Center, The University of Texas
#   at Austin All rights reserved.
#
#   Licensed under the BSD 3-Clause License, (the "License"); you may not use
#   this file except in compliance with the License. A copy of the License is
#   included with this software in the file LICENSE. If your copy does not
#   contain the License, you may obtain a copy of the License at:
#
#   http://opensource.org/licenses/BSD-3-Clause
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#   WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#   License for the specific language governing permissions and limitations under
#   limitations under the License.
#
#   TCP Bridged Display Wall funded in part by an Intel Visualization Center of
#   Excellence award
#   =======================================================================================
#   @author Joao Barbosa <jbarbosa@tacc.utexas.edu>
#*/

set(SNAPPY_HOME "$ENV{SNAPPY_HOME}" CACHE PATH "Google snappy compression algorithm")


if( NOT "${SNAPPY_HOME}" STREQUAL "")
    file( TO_CMAKE_PATH "${SNAPPY_HOME}" _native_path )
    list( APPEND _snappy_roots ${_native_path} )
elseif ( Snappy_HOME )
    list( APPEND _snappy_roots ${Snappy_HOME} )
endif()

message(STATUS "SNAPPY_HOME: ${SNAPPY_HOME}")
find_path(SNAPPY_INCLUDE_DIR snappy.h HINTS
  ${_snappy_roots}
  NO_DEFAULT_PATH
  PATH_SUFFIXES "include")

find_library(SNAPPY_LIBRARIES NAMES snappy PATHS
  ${_snappy_roots}
  NO_DEFAULT_PATH
  PATH_SUFFIXES "lib")

if (SNAPPY_INCLUDE_DIR AND (PARQUET_MINIMAL_DEPENDENCY OR SNAPPY_LIBRARIES))
  set(SNAPPY_FOUND TRUE)
  get_filename_component( SNAPPY_LIBS ${SNAPPY_LIBRARIES} PATH )
  set(SNAPPY_HEADER_NAME snappy.h)
  set(SNAPPY_HEADER ${SNAPPY_INCLUDE_DIR}/${SNAPPY_HEADER_NAME})
  set(SNAPPY_LIB_NAME snappy)
  set(SNAPPY_STATIC_LIB ${SNAPPY_LIBS}/${CMAKE_STATIC_LIBRARY_PREFIX}${SNAPPY_LIB_NAME}${SNAPPY_MSVC_STATIC_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX})
  set(SNAPPY_SHARED_LIB ${SNAPPY_LIBS}/${CMAKE_SHARED_LIBRARY_PREFIX}${SNAPPY_LIB_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
else ()
  set(SNAPPY_FOUND FALSE)
endif ()

if (SNAPPY_FOUND)
  if (NOT Snappy_FIND_QUIETLY)
    if (PARQUET_MINIMAL_DEPENDENCY)
      message(STATUS "Found the Snappy header: ${SNAPPY_HEADER}")
    else ()
      message(STATUS "Found the Snappy library: ${SNAPPY_LIBRARIES}")
    endif ()
  endif ()
else ()
  if (NOT Snappy_FIND_QUIETLY)
    set(SNAPPY_ERR_MSG "Could not find the Snappy library. Looked in ")
    if ( _snappy_roots )
      set(SNAPPY_ERR_MSG "${SNAPPY_ERR_MSG} in ${_snappy_roots}.")
    else ()
      set(SNAPPY_ERR_MSG "${SNAPPY_ERR_MSG} system search paths.")
    endif ()
    if (Snappy_FIND_REQUIRED)
      message(FATAL_ERROR "${SNAPPY_ERR_MSG}")
    else (Snappy_FIND_REQUIRED)
      message(STATUS "${SNAPPY_ERR_MSG}")
    endif (Snappy_FIND_REQUIRED)
  endif ()
endif ()

mark_as_advanced(
  SNAPPY_INCLUDE_DIR
  SNAPPY_LIBS
  SNAPPY_LIBRARIES
  SNAPPY_STATIC_LIB
  SNAPPY_SHARED_LIB
)
