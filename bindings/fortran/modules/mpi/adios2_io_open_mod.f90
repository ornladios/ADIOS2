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

    interface adios2_open
        module procedure adios2_open_old_comm
        module procedure adios2_open_new_comm
    end interface

contains

    subroutine adios2_open_old_comm(engine, io, name, adios2_mode, ierr)
        integer(kind=8), intent(out) :: engine
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: adios2_mode
        integer, intent(out) :: ierr

        call adios2_open_f2c(engine, io, TRIM(ADJUSTL(name))//char(0), &
                             adios2_mode, ierr)

    end subroutine

    subroutine adios2_open_new_comm(engine, io, name, adios2_mode, comm, ierr)
        integer(kind=8), intent(out) :: engine
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: adios2_mode
        integer, intent(in) :: comm
        integer, intent(out) :: ierr

        call adios2_open_new_comm_f2c(engine, io, &
                                      TRIM(ADJUSTL(name))//char(0), &
                                      adios2_mode, comm, ierr)

    end subroutine

end module
