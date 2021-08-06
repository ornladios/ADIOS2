!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_adios_init_mod_serial.F90 : ADIOS2 Fortran bindings for ADIOS
!                                     class Init functions (serial variants)
!

#ifdef ADIOS2_HAVE_FORTRAN_SUBMODULES
# define ADIOS2_MODULE_PROCEDURE module
#else
# define ADIOS2_MODULE_PROCEDURE
#endif

#ifdef ADIOS2_HAVE_FORTRAN_SUBMODULES
submodule ( adios2_adios_init_mod ) adios2_adios_init_serial_smod
#else
module adios2_adios_init_serial_mod
#endif

    use adios2_parameters_mod
    use adios2_functions_mod
    implicit none

#ifndef ADIOS2_HAVE_FORTRAN_SUBMODULES
    interface adios2_init
        module procedure adios2_init_serial
        module procedure adios2_init_debug_serial
        module procedure adios2_init_config_serial
        module procedure adios2_init_config_debug_serial
    end interface
#endif
    external adios2_init_config_serial_f2c

contains

    ADIOS2_MODULE_PROCEDURE subroutine adios2_init_serial( &
            adios, adios2_debug_mode, ierr)
        type(adios2_adios), intent(out) :: adios
        logical, intent(in) :: adios2_debug_mode
        integer, intent(out) :: ierr

        call adios2_init_config_serial(adios, char(0), adios2_debug_mode, ierr)

    end subroutine

    ADIOS2_MODULE_PROCEDURE subroutine adios2_init_debug_serial(adios, ierr)
        type(adios2_adios), intent(out) :: adios
        integer, intent(out) :: ierr

        call adios2_init_config_serial(adios, char(0), .true., ierr)

    end subroutine

    ADIOS2_MODULE_PROCEDURE subroutine adios2_init_config_serial( &
            adios, config_file, adios2_debug_mode, ierr)
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

    ADIOS2_MODULE_PROCEDURE subroutine adios2_init_config_debug_serial( &
            adios, config_file, ierr)
        type(adios2_adios), intent(out) :: adios
        character*(*), intent(in) :: config_file
        integer, intent(out) :: ierr

        call adios2_init_config_serial(adios, config_file, .true., ierr)

    end subroutine

#ifdef ADIOS2_HAVE_FORTRAN_SUBMODULES
end submodule
#else
end module
#endif
