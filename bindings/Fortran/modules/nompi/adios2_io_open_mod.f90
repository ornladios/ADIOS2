!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_io_open_mod.f90 : ADIOS2 Fortran bindings for IO class open
!                                 function
!   Created on: Mar 13, 2017
!       Author: William F Godoy godoywf@ornl.gov
!

module adios2_io_open_mod
    use adios2_parameters_mod
    implicit none

contains

    subroutine adios2_open(engine, io, name, adios2_mode, ierr)
        type(adios2_engine), intent(out) :: engine
        type(adios2_io), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: adios2_mode
        integer, intent(out) :: ierr

        call adios2_open_f2c(engine%f2c, io%f2c, TRIM(ADJUSTL(name))//char(0), &
                             adios2_mode, ierr)

        if( ierr == 0 ) then
            engine%valid = .true.
            engine%name = name
            engine%type = io%engine_type
            engine%mode = adios2_mode
        end if

    end subroutine

end module
