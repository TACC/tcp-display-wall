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

IF (TARGET ospray_module_dwcommon)
    # this target is already built!
ELSE ()


    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    set(COMPRESSION_LIB "")

    option(DW_USE_SNAPPY OFF "Use Google SNAPPY algorithm")
    if(DW_USE_SNAPPY)
        FIND_PACKAGE(SNAPPY)
        include_directories(${SNAPPY_INCLUDE_DIR})
        add_definitions("-DDW_USE_SNAPPY")
        set(COMPRESSION_LIB ${SNAPPY_SHARED_LIB})
    endif()

    option(DW_USE_DENSITY OFF "Use Density algorithm")
    if(DW_USE_DENSITY)
        OPTION(DENSITY_USE_STATIC_LIBS ON "Density use static lib")
        OPTION(DENSITY_MEASURE_TIMES OFF "Measure communication times")
        FIND_PACKAGE(DENSITY)
        include_directories(${DENSITY_INCLUDE_DIR})
        link_directories(${DENSITY_LIBRARY_DIR})
        add_definitions("-DDW_USE_DENSITY")
        if(DENSITY_MEASURE_TIMES)
            add_definitions("-DDENSITY_MEASURE_TIMES")
        endif()
        set(COMPRESSION_LIB ${DENSITY_LIBRARIES})
    endif()



    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    ospray_create_library(ospray_module_dwcommon
            networking/TCPFabric.cpp
            work/DWwork.cpp

            LINK

            ospray
            ospray_mpi_common
            ospray_module_mpi
            ${COMPRESSION_LIB}

            )
ENDIF ()
