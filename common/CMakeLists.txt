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
