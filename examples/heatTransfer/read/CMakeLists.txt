#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS_HAVE_MPI)

    find_package(MPI COMPONENTS C REQUIRED)
    add_executable(heatTransfer_read_adios2 heatRead_adios2.cpp PrintData.cpp)
    target_include_directories(heatTransfer_read_adios2
      PRIVATE ${MPI_C_INCLUDE_PATH}
    )
    target_link_libraries(heatTransfer_read_adios2
      adios2 ${MPI_C_LIBRARIES}
    )
    
  if(ADIOS_HAVE_ADIOS1)
    find_package(ADIOS1 REQUIRED)

    add_executable(heatTransfer_read_adios1 heatRead_adios1.cpp PrintData.cpp)
    target_include_directories(heatTransfer_read_adios1
      PRIVATE ${MPI_C_INCLUDE_PATH}
    )
    target_link_libraries(heatTransfer_read_adios1
      adios1::adios ${MPI_C_LIBRARIES}
    )
  endif()
  
endif()

