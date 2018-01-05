!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_io_open_mod.f90 : ADIOS2 Fortran bindings for IO class open function
!
!   Created on: Mar 13, 2017
!       Author: William F Godoy godoywf@ornl.gov
!

module adios2_fopen_mod
    implicit none

    interface adios2_fopen
        module procedure adios2_fopen_config
        module procedure adios2_fopen_default
    end interface

contains

    subroutine adios2_fopen_config(unit, name, adios2_mode, comm, &
                                   config_file, io_in_config_file, ierr)
        integer(kind=8), intent(out) :: unit
        character*(*), intent(in) :: name
        integer, intent(in) :: adios2_mode
        integer, intent(in) :: comm
        character*(*), intent(in) :: config_file
        character*(*), intent(in) :: io_in_config_file
        integer, intent(out) :: ierr

        call adios2_fopen_f2c(unit, TRIM(ADJUSTL(name))//char(0), &
                              adios2_mode, comm, &
                              TRIM(ADJUSTL(config_file))//char(0), &
                              TRIM(ADJUSTL(io_in_config_file))//char(0), &
                              ierr)
    end subroutine

    subroutine adios2_fopen_default(unit, name, adios2_mode, comm, ierr)
        integer(kind=8), intent(out) :: unit
        character*(*), intent(in) :: name
        integer, intent(in) :: adios2_mode
        integer, intent(in) :: comm
        integer, intent(out) :: ierr

        call adios2_fopen_f2c(unit, TRIM(ADJUSTL(name))//char(0), adios2_mode, &
                              comm, ierr)
    end subroutine

end module
