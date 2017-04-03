#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

#add_executable(heatRead_adios heatRead_adios.cpp)
#
#if(ADIOS_USE_MPI)
#    find_package(MPI COMPONENTS C REQUIRED)
#    target_include_directories(heatRead_adios PRIVATE ${MPI_C_INCLUDE_PATH})
#    target_link_libraries(heatRead_adios adios2 ${MPI_C_LIBRARIES})
#else(ADIOS_USE_MPI) 
#    target_link_libraries(heatRead_adios adios2)
#endif()

if(ADIOS_USE_ADIOS1)
    find_package(ADIOS1 REQUIRED)
    add_executable(heatRead_adios1 heatRead_adios1.cpp)
    if(ADIOS_USE_MPI)
        target_include_directories(heatRead_adios1 PRIVATE ${MPI_C_INCLUDE_PATH})
        target_link_libraries(heatRead_adios1  adios1::adios ${MPI_C_LIBRARIES})
    else(ADIOS_USE_MPI) 
        target_compile_definitions(heatRead_adios1 PRIVATE -D_NOMPI)
        target_link_libraries(heatRead_adios1 adios1::adios)
    endif()
endif()

