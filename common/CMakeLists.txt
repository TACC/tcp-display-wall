IF (TARGET ospray_module_dwcommon)
    # this target is already built!
ELSE ()


    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    FIND_PACKAGE(SNAPPY)
        include_directories(${SNAPPY_INCLUDE_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    ospray_create_library(ospray_module_dwcommon
            networking/TCPFabric.cpp
            work/DWwork.cpp

            LINK
            ospray
            ospray_mpi_common
            ospray_module_mpi
            ${SNAPPY_STATIC_LIB}
            )
ENDIF ()