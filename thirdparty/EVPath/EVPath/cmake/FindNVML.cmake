
set( NVML_LIB_PATHS /usr/lib64 )
if(GPU_DEPLOYMENT_KIT_ROOT_DIR)
    list(APPEND NVML_LIB_PATHS "${GPU_DEPLOYMENT_KIT_ROOT_DIR}/src/gdk/nvml/lib")
endif()
set(NVML_NAMES nvidia-ml)
  
set( NVML_INC_PATHS /usr/include/nvidia/gdk/ /usr/include )
if(GPU_DEPLOYMENT_KIT_ROOT_DIR)
    list(APPEND NVML_INC_PATHS "${GPU_DEPLOYMENT_KIT_ROOT_DIR}/include/nvidia/gdk")
endif()

find_library(NVML_LIBRARY NAMES ${NVML_NAMES} PATHS ${NVML_LIB_PATHS} )

find_path(NVML_INCLUDE_DIR nvml.h PATHS ${NVML_INC_PATHS})

# handle the QUIETLY and REQUIRED arguments and set NVML_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NVML DEFAULT_MSG NVML_LIBRARY NVML_INCLUDE_DIR)

mark_as_advanced(NVML_LIBRARY NVML_INCLUDE_DIR)
