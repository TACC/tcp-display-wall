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


    SET(DW_SEND_BUFFER_SIZE 64 CACHE STRING "Number of tiles in send buffer")
    SET_PROPERTY(CACHE DW_SEND_BUFFER_SIZE PROPERTY STRINGS 8 16 32 64 128 256 512)

    include_directories(${CMAKE_CURRENT_SOURCE_DIR})

    add_definitions(-DDW_SEND_BUFFER_SIZE=${DW_SEND_BUFFER_SIZE})

    ospray_create_library(ospray_module_dwfarm
            dw_farm_init.cpp
            Device.cpp
            fb/FarmFramebuffer.cpp
            work/FarmWork.cpp


            LINK

            ospray
            ospray_mpi_common
            ospray_module_mpi
            ospray_module_dwcommon
            ospray_module_ispc
            )

    ospray_create_application(
            dwFarm
            main.cpp

            LINK
            ospray
            ospray_mpi_common
            ospray_module_mpi
            ospray_module_dwcommon
            ospray_module_ispc
            ${GLFW_LIBRARY}
    )

