#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(heatTransfer_adios main.cpp HeatTransfer.cpp Settings.cpp IO_adios2.cpp)

if(ADIOS_USE_MPI)
    find_package(MPI COMPONENTS C REQUIRED)
    target_include_directories(heatTransfer_adios PRIVATE ${MPI_C_INCLUDE_PATH})
    target_link_libraries(heatTransfer_adios adios2 ${MPI_C_LIBRARIES})
else(ADIOS_USE_MPI) 
    add_executable(heatTransfer_adios main.cpp HeatTransfer.cpp Settings.cpp IO_adios2.cpp)
    target_link_libraries(heatTransfer_adios adios2)
endif()

if(ADIOS_USE_ADIOS1)
    add_executable(heatTransfer_adios1 main.cpp HeatTransfer.cpp Settings.cpp IO_adios1.cpp)
endif()

#add_subdirectory(read)
