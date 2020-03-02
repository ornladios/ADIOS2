!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_adios_init_mod.F90 : ADIOS2 Fortran bindings for ADIOS class Init
!                              functions
!

module adios2_adios_init_mod
    use adios2_adios_init_mod_serial
#ifdef ADIOS2_HAVE_MPI_F
    use adios2_adios_init_mod_mpi
#endif
end module
