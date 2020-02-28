!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_adios_init_mod.f90 : ADIOS2 Fortran bindings for ADIOS class Init
!                              functions
!

module adios2_adios_init_mod
    use adios2_parameters_mod
    use adios2_functions_mod
    implicit none

    interface adios2_init
        module procedure adios2_init_serial
        module procedure adios2_init_debug_serial
        module procedure adios2_init_config_serial
        module procedure adios2_init_config_debug_serial
#ifdef ADIOS2_HAVE_MPI_F
        module procedure adios2_init_mpi
        module procedure adios2_init_debug_mpi
        module procedure adios2_init_config_mpi
        module procedure adios2_init_config_debug_mpi
#endif
    end interface

contains

    subroutine adios2_init_serial(adios, adios2_debug_mode, ierr)
        type(adios2_adios), intent(out) :: adios
        logical, intent(in) :: adios2_debug_mode
        integer, intent(out) :: ierr

        call adios2_init_config_serial(adios, char(0), adios2_debug_mode, ierr)

    end subroutine

    subroutine adios2_init_debug_serial(adios, ierr)
        type(adios2_adios), intent(out) :: adios
        integer, intent(out) :: ierr

        call adios2_init_config_serial(adios, char(0), .true., ierr)

    end subroutine

    subroutine adios2_init_config_serial(adios, config_file, adios2_debug_mode, ierr)
        type(adios2_adios), intent(out) :: adios
        character*(*), intent(in) :: config_file
        logical, intent(in) :: adios2_debug_mode
        integer, intent(out) :: ierr
        ! local
        integer debug_mode

        debug_mode = adios2_LogicalToInt(adios2_debug_mode)
        call adios2_init_config_serial_f2c(adios%f2c, config_file, debug_mode, ierr)
        if( ierr == 0 ) adios%valid = .true.

    end subroutine

    subroutine adios2_init_config_debug_serial(adios, config_file, ierr)
        type(adios2_adios), intent(out) :: adios
        character*(*), intent(in) :: config_file
        integer, intent(out) :: ierr

        call adios2_init_config_serial(adios, config_file, .true., ierr)

    end subroutine

#ifdef ADIOS2_HAVE_MPI_F

    subroutine adios2_init_mpi(adios, comm, adios2_debug_mode, ierr)
        type(adios2_adios), intent(out) :: adios
        integer, intent(in) :: comm
        logical, intent(in) :: adios2_debug_mode
        integer, intent(out) :: ierr

        call adios2_init_config_mpi(adios, char(0), comm, adios2_debug_mode, ierr)

    end subroutine

    subroutine adios2_init_debug_mpi(adios, comm, ierr)
        type(adios2_adios), intent(out) :: adios
        integer, intent(in) :: comm
        integer, intent(out) :: ierr

        call adios2_init_config_mpi(adios, char(0), comm, .true., ierr)

    end subroutine

    subroutine adios2_init_config_mpi(adios, config_file, comm, adios2_debug_mode, &
                                      ierr)
        type(adios2_adios), intent(out) :: adios
        character*(*), intent(in) :: config_file
        integer, intent(in) :: comm
        logical, intent(in) :: adios2_debug_mode
        integer, intent(out) :: ierr
        ! local
        integer debug_mode

        debug_mode = adios2_LogicalToInt(adios2_debug_mode)
        call adios2_init_config_mpi_f2c(adios%f2c, &
                                        TRIM(ADJUSTL(config_file))//char(0), &
                                        comm, debug_mode, ierr)
        if( ierr == 0 ) adios%valid = .true.

    end subroutine


    subroutine adios2_init_config_debug_mpi(adios, config_file, comm, ierr)
        type(adios2_adios), intent(out) :: adios
        character*(*), intent(in) :: config_file
        integer, intent(in) :: comm
        integer, intent(out) :: ierr

        call adios2_init_config_mpi(adios, config_file, comm, .true., ierr)

    end subroutine

#endif

end module
