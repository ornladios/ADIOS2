!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_io_open_mod.f90 : ADIOS2 Fortran bindings for IO class open function
!
!   Created on: Mar 13, 2017
!       Author: William F Godoy godoywf@ornl.gov
!

module adios2_io_open_mod
    use adios2_parameters_mod
    implicit none

    interface adios2_open
        module procedure adios2_open_old_comm
        module procedure adios2_open_new_comm
    end interface

contains

    subroutine adios2_open_old_comm(engine, io, name, adios2_mode, ierr)
        type(adios2_engine), intent(out) :: engine
        type(adios2_io), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: adios2_mode
        integer, intent(out) :: ierr

        engine%mode = adios2_mode
        call adios2_open_f2c(engine%f2c, io%f2c, TRIM(ADJUSTL(name))//char(0), &
                             adios2_mode, ierr)

        if( ierr == 0 ) then
            engine%valid = .true.
            engine%name = name
            call adios2_engine_get_type_f2c(engine%type, engine%f2c, ierr)
            engine%mode = adios2_mode
        end if

    end subroutine

    subroutine adios2_open_new_comm(engine, io, name, adios2_mode, comm, ierr)
        type(adios2_engine), intent(out) :: engine
        type(adios2_io), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: adios2_mode
        integer, intent(in) :: comm
        integer, intent(out) :: ierr

        call adios2_open_new_comm_f2c(engine%f2c, io%f2c, &
                                      TRIM(ADJUSTL(name))//char(0), &
                                      adios2_mode, comm, ierr)

        if( ierr == 0 ) then
            engine%valid = .true.
            engine%name = name
            call adios2_engine_get_type_f2c(engine%type, engine%f2c, ierr)
            engine%mode = adios2_mode
        end if

    end subroutine

end module
