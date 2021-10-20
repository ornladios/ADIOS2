#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_library(adios2_h5vol
  H5VolReadWrite.c
  H5Vol.c
  H5VolUtil.c
  H5Vol_attr.c
  H5Vol_dataset.c
  H5Vol_file.c
  H5Vol_group.c
  H5Vol_link.c
  H5Vol_object.c
)
target_include_directories(adios2_h5vol PRIVATE ${HDF5_C_INCLUDE_DIRS})	 
target_link_libraries(adios2_h5vol PRIVATE
  ${HDF5_C_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}
)
if(NOT HDF5_IS_PARALLEL)
  target_link_libraries(adios2_h5vol PRIVATE adios2::c)
elseif(HDF5_IS_PARALLEL AND ADIOS2_HAVE_MPI)
  target_link_libraries(adios2_h5vol PRIVATE adios2::c_mpi MPI::MPI_C)
else() # HDF5_IS_PARALLEL AND NOT ADIOS2_HAVE_MPI
  message(FATAL_ERROR "Invalid configuration: HDF5_IS_PARALLEL AND NOT ADIOS2_HAVE_MPI")
endif()

install(FILES H5Vol_def.h H5VolError.h H5Vol.h H5VolReadWrite.h H5VolUtil.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adios2/h5vol
  COMPONENT adios2_h5vol
)
install(TARGETS adios2_h5vol
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  COMPONENT adios2_h5vol
)
