!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_io_open_mod.f90 : ADIOS2 Fortran bindings for IO class open function
!
!   Created on: Mar 13, 2017
!       Author: William F Godoy godoywf@ornl.gov
!

module adios2_io_open
    implicit none

contains

    subroutine adios2_open(engine, io, name, adios2_open_mode, ierr)
        integer(kind=8), intent(out) :: engine
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: adios2_open_mode
        integer, intent(out) :: ierr

        call adios2_open_f2c(engine, io, TRIM(ADJUSTL(name))//char(0), &
            & adios2_open_mode, ierr)

    end subroutine


    subroutine adios2_open_new_comm(engine, io, name, adios2_open_mode, comm, &
        & ierr)
        integer(kind=8), intent(out) :: engine
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: adios2_open_mode
        integer, intent(in) :: comm
        integer, intent(out) :: ierr

        call adios2_open_new_comm_f2c(engine, io, TRIM(ADJUSTL(name))//char(0),&
            & adios2_open_mode, comm, ierr)

    end subroutine

end module
