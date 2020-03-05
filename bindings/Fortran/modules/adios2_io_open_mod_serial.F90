!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_io_open_mod_serial.F90 : ADIOS2 Fortran bindings for IO
!                                  class open function (serial variants)
!

module adios2_io_open_mod_serial
    use adios2_parameters_mod
    implicit none

    interface adios2_open
        module procedure adios2_open_old_comm
    end interface

contains

    subroutine adios2_open_old_comm(engine, io, name, adios2_mode, ierr)
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
            call adios2_engine_get_type_f2c(engine%type, engine%f2c, ierr)
            engine%mode = adios2_mode
        end if

    end subroutine

end module
