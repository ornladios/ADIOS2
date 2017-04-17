#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS_USE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)

  add_executable(heatTransfer_write_adios2
    main.cpp
    HeatTransfer.cpp
    Settings.cpp
    IO_adios2.cpp
  )
  target_include_directories(heatTransfer_write_adios2
    PRIVATE ${MPI_C_INCLUDE_PATH}
  )
  target_link_libraries(heatTransfer_write_adios2 adios2 ${MPI_C_LIBRARIES})

  if(ADIOS_USE_ADIOS1)
    find_package(ADIOS1 REQUIRED)
    find_package(MPI COMPONENTS C REQUIRED)

    add_executable(heatTransfer_write_adios1
      main.cpp
      HeatTransfer.cpp
      Settings.cpp
      IO_adios1.cpp
    )
    target_include_directories(heatTransfer_write_adios1
      PRIVATE ${MPI_C_INCLUDE_PATH}
    )
    target_link_libraries(heatTransfer_write_adios1
      adios1::adios ${MPI_C_LIBRARIES}
    )
  endif()
endif()
