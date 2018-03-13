!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_stream_open_mod.f90 : ADIOS2 Fortran bindings for high-level Stream
!                               class
!   Created on: Feb 13, 2018
!       Author: William F Godoy godoywf@ornl.gov
!

module adios2_fopen_mod
    use adios2_parameters
    implicit none

    interface adios2_fopen
        module procedure adios2_fopen_config
        module procedure adios2_fopen_default
    end interface

contains

    subroutine adios2_fopen_full(unit, name, adios2_mode, config_file, &
                                 io_in_config_file, ierr)
        type(adios2_file), intent(out):: unit
        character*(*), intent(in) :: name
        integer, intent(in) :: adios2_mode
        character*(*), intent(in) :: config_file
        character*(*), intent(in) :: io_in_config_file
        integer, intent(out) :: ierr

        call adios2_fopen_f2c(unit%fh, TRIM(ADJUSTL(name))//char(0), adios2_mode, &
                              TRIM(ADJUSTL(config_file))//char(0), &
                              TRIM(ADJUSTL(io_in_config_file))//char(0), ierr)
    end subroutine

    subroutine adios2_fopen_default(unit, name, adios2_mode, ierr)
        type(adios2_file), intent(out):: unit
        character*(*), intent(in) :: name
        integer, intent(in) :: adios2_mode
        integer, intent(out) :: ierr

        call adios2_fopen_f2c(unit%fh, TRIM(ADJUSTL(name))//char(0), adios2_mode, &
                              ierr)
    end subroutine

end module
