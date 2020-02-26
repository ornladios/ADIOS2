!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_io_open_mod.F90 : ADIOS2 Fortran bindings for IO class open function
!

module adios2_io_open_mod
    use adios2_io_open_mod_serial
#ifdef ADIOS2_HAVE_MPI_F
    use adios2_io_open_mod_mpi
#endif
end module
