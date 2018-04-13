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

set(DENSITY_DIR "$ENV{DENSITY_DIR}" CACHE PATH "Density compression algorithm")

if("${DENSITY_DIR}" STREQUAL "")
  message(ERROR "Please specify density directory")
else()

  SET(SEARCH_SUFFIXES build/ src/ lib/ lib64/ include/)


  #Check whether to search static or dynamic libs
  set( CMAKE_FIND_LIBRARY_SUFFIXES_SAV ${CMAKE_FIND_LIBRARY_SUFFIXES} )


  find_library(DENSITY_SHARED_LIB
          ${CMAKE_SHARED_LIBRARY_PREFIX}density${CMAKE_SHARED_LIBRARY_SUFFIX}
          PATHS
          ${DENSITY_DIR}
          PATH_SUFFIXES
          ${SEARCH_SUFFIXES}
          NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
          )

  find_library(DENSITY_STATIC_LIB
          ${CMAKE_STATIC_LIBRARY_PREFIX}density${CMAKE_STATIC_LIBRARY_SUFFIX}
          PATHS
          ${DENSITY_DIR}
          PATH_SUFFIXES
          ${SEARCH_SUFFIXES}
          NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
          )

  find_path(DENSITY_INCLUDE_DIR
          density_api.h
          HINTS
          ${DENSITY_DIR}
          PATHS
          ${DENSITY_DIR}
          PATH_SUFFIXES
          ${SEARCH_SUFFIXES}
          NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)

  get_filename_component(DENSITY_LIBRARY_DIR ${DENSITY_SHARED_LIB} PATH)

  if(NOT DENSITY_LIBRARY_DIR EQUAL "")
    SET(DENSITY_FOUND TRUE)
    if(DENSITY_USE_STATIC_LIBS)
      set(DENSITY_LIBRARIES ${DENSITY_STATIC_LIB} )
    else()
      set(DENSITY_LIBRARIES ${DENSITY_SHARED_LIB} )
    endif()

    mark_as_advanced(
            DENSITY_INCLUDE_DIR
            DENSITY_LIBS
            DENSITY_LIBRARIES
            DENSITY_LIBRARY_DIR
            DENSITY_STATIC_LIB
            DENSITY_SHARED_LIB
    )

  else()
    SET(DENSITY_FOUND FALSE)
    MESSAGE(ERROR "Could not find DENSITY")
  endif()

endif()


