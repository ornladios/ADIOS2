#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(heatTransfer_read heatRead.cpp PrintDataStep.h)
  target_link_libraries(heatTransfer_read adios2 MPI::MPI_C)
  target_compile_definitions(heatTransfer_read PRIVATE
   -DDEFAULT_CONFIG=${CMAKE_CURRENT_SOURCE_DIR}/../heat.xml)
endif()
