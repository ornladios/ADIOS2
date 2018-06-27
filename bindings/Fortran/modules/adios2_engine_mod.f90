!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_engine_mod.f90 : ADIOS2 Fortran bindings for Engine class
!
!   Created on: Aug 22, 2017
!       Author: William F Godoy godoywf@ornl.gov
!

module adios2_engine_mod
    use adios2_engine_begin_step_mod
    use adios2_engine_put_mod
    use adios2_engine_get_mod
    implicit none

contains

    subroutine adios2_perform_puts(engine, ierr)
        type(adios2_engine), intent(in) :: engine
        integer, intent(out) :: ierr

        call adios2_perform_puts_f2c(engine%f2c, ierr)

    end subroutine

    subroutine adios2_perform_gets(engine, ierr)
        type(adios2_engine), intent(in) :: engine
        integer, intent(out) :: ierr

        call adios2_perform_gets_f2c(engine%f2c, ierr)

    end subroutine

    subroutine adios2_end_step(engine, ierr)
        type(adios2_engine), intent(in) :: engine
        integer, intent(out) :: ierr

        call adios2_end_step_f2c(engine%f2c, ierr)

    end subroutine

    subroutine adios2_flush(engine, ierr)
        type(adios2_engine), intent(in) :: engine
        integer, intent(out) :: ierr

        call adios2_flush_f2c(engine%f2c, ierr)

    end subroutine

    subroutine adios2_close(engine, ierr)
        type(adios2_engine), intent(inout) :: engine
        integer, intent(out) :: ierr

        call adios2_close_f2c(engine%f2c, ierr)

        if( ierr == 0 ) then
            engine%valid = .false.
            engine%type = ''
            engine%mode = adios2_mode_undefined
        end if

    end subroutine

    subroutine adios2_current_step(engine, current_step, ierr)
        type(adios2_engine), intent(in) :: engine
        integer(kind=8), intent(out) :: current_step
        integer, intent(out) :: ierr

        call adios2_current_step_f2c(engine%f2c, current_step, ierr)

    end subroutine

end module
