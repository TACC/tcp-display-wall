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
option(OSPRAY_MODULE_DISPLAYWALL "Build DiplayWall Module" ON)
IF(OSPRAY_MODULE_DISPLAYWALL)

    OSPRAY_CONFIGURE_MPI()
    OSPRAY_BUILD_COMPONENT(mpiCommon)
    OSPRAY_BUILD_COMPONENT(mpiMessageLayer)
    ospray_disable_compiler_warnings()
    find_package(glfw3 QUIET)
#    find_package(OpenGL REQUIRED)

    if (NOT GLFW_FOUND)
        set(GLFW_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/common/glfw/include PARENT_SCOPE)
        set(GLFW_LIBRARY glfw PARENT_SCOPE)
        set(GLFW_LIBRARIES ${GLFW_LIBRARY} PARENT_SCOPE)
    else()
        set(GLFW_INCLUDE_DIRS ${GLFW_INCLUDE_DIRS} PARENT_SCOPE)
        set(GLFW_LIBRARY ${GLFW_LIBRARY} PARENT_SCOPE)
        set(GLFW_LIBRARIES ${GLFW_LIBRARY} PARENT_SCOPE)
        message(STATUS "Using GLFW found in the environment")
    endif()

    include_directories(${GLFW_INCLUDE_DIRS})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

    add_subdirectory(common)
    add_subdirectory(farm)
    add_subdirectory(display)


ENDIF(OSPRAY_MODULE_DISPLAYWALL)
