
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

ospray_create_library(ospray_module_dwdisplay dw_display_init.cpp Device.cpp
		work/OSPWork.cpp
		fb/DisplayFramebuffer.cpp
		glDisplay/glDisplay.cpp
		glDisplay/WallConfig.cpp
		LINK
		ospray
		ospray_mpi_common
		ospray_module_mpi
		ospray_module_dwcommon ospray_module_ispc
		${GLFW_LIBRARY} ${OPENGL_LIBRARIES})

ospray_create_application(
		dwDisplay
		main.cpp
		LINK ospray ospray_mpi_common ospray_module_mpi
		ospray_module_dwcommon ospray_module_ispc
		ospray_module_dwdisplay ${GLFW_LIBRARY} ${OPENGL_LIBRARIES})
