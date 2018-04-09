option(OSPRAY_MODULE_DISPLAYWALL "Build DiplayWall Module" ON)
IF(OSPRAY_MODULE_DISPLAYWALL)

    OSPRAY_CONFIGURE_MPI()
    OSPRAY_BUILD_COMPONENT(mpiCommon)
    OSPRAY_BUILD_COMPONENT(mpiMessageLayer)
    ospray_disable_compiler_warnings()
    find_package(GLFW)
    find_package(OpenGL REQUIRED)

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

    option(DW_USE_UDT OFF "Use UDT Communication library")
    IF(DW_USE_UDT)
        add_definitions("-DDW_USE_UDT")
    ENDIF(DW_USE_UDT)

    add_subdirectory(common)
    add_subdirectory(farm)
    add_subdirectory(display)


ENDIF(OSPRAY_MODULE_DISPLAYWALL)
