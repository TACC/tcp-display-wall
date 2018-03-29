
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})

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

