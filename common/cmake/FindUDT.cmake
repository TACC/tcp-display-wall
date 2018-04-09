set(UDT_DIR "$ENV{UDT_DIR}" CACHE PATH "Google udt compression algorithm")


if( NOT "${UDT_DIR}" STREQUAL "")
    file( TO_CMAKE_PATH "${UDT_DIR}" _native_path )
    list( APPEND _udt_roots ${_native_path} )
elseif ( UDT_HOME )
    list( APPEND _udt_roots ${UDT_HOME} )
endif()

message(STATUS "UDT_DIR: ${UDT_DIR}")
find_path(UDT_INCLUDE_DIR udt4/api.h HINTS
  ${_udt_roots}
  NO_DEFAULT_PATH
  PATH_SUFFIXES "include")

find_library(UDT_LIBRARIES NAMES udt PATHS
  ${_udt_roots}
  NO_DEFAULT_PATH
  PATH_SUFFIXES "lib")

if (UDT_INCLUDE_DIR AND (PARQUET_MINIMAL_DEPENDENCY OR UDT_LIBRARIES))
  set(UDT_FOUND TRUE)
  get_filename_component( UDT_LIBS ${UDT_LIBRARIES} PATH )
  set(UDT_HEADER_NAME udt4/api.h)
  set(UDT_HEADER ${UDT_INCLUDE_DIR}/${UDT_HEADER_NAME})
  set(UDT_LIB_NAME udt)
  set(UDT_STATIC_LIB ${UDT_LIBS}/${CMAKE_STATIC_LIBRARY_PREFIX}${UDT_LIB_NAME}${UDT_MSVC_STATIC_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX})
  set(UDT_SHARED_LIB ${UDT_LIBS}/${CMAKE_SHARED_LIBRARY_PREFIX}${UDT_LIB_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
else ()
  set(UDT_FOUND FALSE)
endif ()

if (UDT_FOUND)
  if (NOT UDT_FIND_QUIETLY)
    if (PARQUET_MINIMAL_DEPENDENCY)
      message(STATUS "Found the UDT header: ${UDT_HEADER}")
    else ()
      message(STATUS "Found the UDT library: ${UDT_LIBRARIES}")
    endif ()
  endif ()
else ()
  if (NOT UDT_FIND_QUIETLY)
    set(UDT_ERR_MSG "Could not find the UDT library. Looked in ")
    if ( _udt_roots )
      set(UDT_ERR_MSG "${UDT_ERR_MSG} in ${_udt_roots}.")
    else ()
      set(UDT_ERR_MSG "${UDT_ERR_MSG} system search paths.")
    endif ()
    if (UDT_FIND_REQUIRED)
      message(FATAL_ERROR "${UDT_ERR_MSG}")
    else (UDT_FIND_REQUIRED)
      message(STATUS "${UDT_ERR_MSG}")
    endif (UDT_FIND_REQUIRED)
  endif ()
endif ()

mark_as_advanced(
  UDT_INCLUDE_DIR
  UDT_LIBS
  UDT_LIBRARIES
  UDT_STATIC_LIB
  UDT_SHARED_LIB
)
