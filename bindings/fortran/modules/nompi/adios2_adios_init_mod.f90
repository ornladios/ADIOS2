!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_init_mod.f90 : ADIOS2 Fortran bindings nonMPI init functions
!
!   Created on: Mar 13, 2017
!       Author: William F Godoy godoywf@ornl.gov
!

module adios2_adios_init
    use adios2_functions
    implicit none

contains

    subroutine adios2_init(adios, adios2_debug_mode, ierr)
        integer(kind=8), intent(out) :: adios
        logical, value, intent(in) :: adios2_debug_mode
        integer, intent(out) :: ierr

        call adios2_init_config(adios, "", adios2_debug_mode, ierr)

    end subroutine

    subroutine adios2_init_config(adios, config_file, adios2_debug_mode, ierr)
        integer(kind=8), intent(out) :: adios
        character*(*), intent(in) :: config_file
        logical, value, intent(in) :: adios2_debug_mode
        integer, intent(out) :: ierr
        ! local
        integer debug_mode

        debug_mode = adios2_LogicalToInt(adios2_debug_mode)
        call adios2_init_config_f2c(adios, config_file, debug_mode, ierr)

    end subroutine

end module
