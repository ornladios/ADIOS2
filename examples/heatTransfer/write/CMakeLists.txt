#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
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

  if(ADIOS2_HAVE_ADIOS1)
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

  if(ADIOS2_HAVE_HDF5)
    find_package(HDF5 REQUIRED)
    find_package(MPI COMPONENTS C REQUIRED)

    add_executable(heatTransfer_write_hdf5
      main.cpp
      HeatTransfer.cpp
      Settings.cpp
      IO_hdf5_a.cpp
    )
    target_include_directories(heatTransfer_write_hdf5
      PRIVATE ${MPI_C_INCLUDE_PATH} ${HDF5_C_INCLUDE_DIRS}
    )
    target_link_libraries(heatTransfer_write_hdf5
      ${MPI_C_LIBRARIES} ${HDF5_C_LIBRARIES}
    )
  endif()


  if(ADIOS2_HAVE_HDF5)
    find_package(HDF5 REQUIRED)
    find_package(MPI COMPONENTS C REQUIRED)

    add_executable(heatTransfer_write_ph5
      main.cpp
      HeatTransfer.cpp
      Settings.cpp
      IO_ph5.cpp
    )
    target_include_directories(heatTransfer_write_ph5
      PRIVATE ${MPI_C_INCLUDE_PATH} ${HDF5_C_INCLUDE_DIRS}
    )
    target_link_libraries(heatTransfer_write_ph5
      ${MPI_C_LIBRARIES} ${HDF5_C_LIBRARIES}
    )
  endif()


  if(ADIOS2_HAVE_HDF5)
     find_package(MPI COMPONENTS C REQUIRED)

     add_executable(heatTransfer_write_a2h5
      main.cpp
      HeatTransfer.cpp
      Settings.cpp
      IO_ph5_adios2.cpp
    )

    target_include_directories(heatTransfer_write_a2h5
      PRIVATE ${MPI_C_INCLUDE_PATH}
    )
    #target_link_libraries(heatTransfer_write_a2h5
    #  ${MPI_C_LIBRARIES} 
    #)
    target_link_libraries(heatTransfer_write_a2h5 PUBLIC adios2)

  endif()

endif()
