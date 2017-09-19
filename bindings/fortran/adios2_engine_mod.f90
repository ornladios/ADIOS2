!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_engine_mod.f90 : ADIOS2 Fortran bindings for Engine class
!
!   Created on: Aug 22, 2017
!       Author: William F Godoy godoywf@ornl.gov
!
module adios2_engine
    use adios2_engine_write
    implicit none

contains

    subroutine adios2_advance(engine, ierr)
        integer(kind=8), intent(in) :: engine
        integer, intent(out) :: ierr

        call adios2_advance_f2c(engine, ierr)

    end subroutine


    subroutine adios2_close(engine, ierr)
        integer(kind=8), intent(in) :: engine
        integer, intent(out) :: ierr

        call adios2_close_f2c(engine, ierr)

    end subroutine

end module
