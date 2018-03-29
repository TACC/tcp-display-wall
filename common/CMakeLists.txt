IF (TARGET ospray_module_dwcommon)
    # this target is already built!
ELSE ()

    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    ospray_create_library(ospray_module_dwcommon
            networking/TCPFabric.cpp
            work/DWwork.cpp

            LINK
            ospray
            ospray_mpi_common
            ospray_module_mpi
            )
ENDIF ()