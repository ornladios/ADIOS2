#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_bpWriter helloBPWriter.cpp)
target_link_libraries(hello_bpWriter adios2::cxx11)

add_executable(hello_bpWriter_c helloBPWriter.c)
target_link_libraries(hello_bpWriter_c adios2::c)

add_executable(hello_bpPutDeferred helloBPPutDeferred.cpp)
target_link_libraries(hello_bpPutDeferred adios2::cxx11)

add_executable(hello_bpSubStreams helloBPSubStreams.cpp)
target_link_libraries(hello_bpSubStreams adios2::cxx11)

if(ADIOS2_HAVE_SZ)
  add_executable(hello_bpSZ helloBPSZ.cpp)
  target_link_libraries(hello_bpSZ adios2::cxx11)
endif()

if(ADIOS2_HAVE_Fortran)
  add_executable(hello_bpWriter_f helloBPWriter.F90)
  set_property(TARGET hello_bpWriter_f PROPERTY LINKER_LANGUAGE Fortran)
  target_link_libraries(hello_bpWriter_f adios2::fortran)
endif()

if(ADIOS2_HAVE_MPI)
  add_executable(hello_bpWriter_mpi helloBPWriter.cpp)
  target_link_libraries(hello_bpWriter_mpi adios2::cxx11_mpi MPI::MPI_C)

  add_executable(hello_bpWriter_c_mpi helloBPWriter.c)
  target_link_libraries(hello_bpWriter_c_mpi adios2::c_mpi MPI::MPI_C)

  add_executable(hello_bpPutDeferred_mpi helloBPPutDeferred.cpp)
  target_link_libraries(hello_bpPutDeferred_mpi adios2::cxx11_mpi MPI::MPI_C)

  add_executable(hello_bpSubStreams_mpi helloBPSubStreams.cpp)
  target_link_libraries(hello_bpSubStreams_mpi adios2::cxx11_mpi MPI::MPI_C)

  if(ADIOS2_HAVE_SZ)
    add_executable(hello_bpSZ_mpi helloBPSZ.cpp)
    target_link_libraries(hello_bpSZ_mpi adios2::cxx11_mpi MPI::MPI_C)
  endif()

  if(ADIOS2_HAVE_Fortran)
    add_executable(hello_bpWriter_f_mpi helloBPWriter.F90)
    set_property(TARGET hello_bpWriter_f_mpi PROPERTY LINKER_LANGUAGE Fortran)
    target_link_libraries(hello_bpWriter_f_mpi adios2::fortran_mpi MPI::MPI_Fortran)
  endif()
endif()
