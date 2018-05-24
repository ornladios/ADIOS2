!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_begin_step_mod.f90 : ADIOS2 Fortran bindings for Engine Begin Step
!  subroutine overloads
!
!   Created on: Aug 22, 2017
!       Author: William F Godoy godoywf@ornl.gov
!

module adios2_engine_begin_step_mod
    use adios2_parameters_mod
    implicit none

    interface adios2_begin_step

        module procedure adios2_begin_step_full
        module procedure adios2_begin_step_mode
        module procedure adios2_begin_step_default

    end interface

contains

    subroutine adios2_begin_step_full(engine, step_mode, timeout_seconds, ierr)
        type(adios2_engine), intent(in) :: engine
        integer, value, intent(in) :: step_mode
        real, value, intent(in) :: timeout_seconds
        integer, intent(out) :: ierr

        call adios2_begin_step_f2c(engine%f2c, step_mode, timeout_seconds, ierr)

    end subroutine

    subroutine adios2_begin_step_mode(engine, step_mode, ierr)
        type(adios2_engine), intent(in) :: engine
        integer, value, intent(in) :: step_mode
        integer, intent(out) :: ierr

        call adios2_begin_step_f2c(engine%f2c, step_mode, 0._4, ierr)

    end subroutine

    subroutine adios2_begin_step_default(engine, ierr)
        type(adios2_engine), intent(in) :: engine
        integer, intent(out) :: ierr

        if( engine%mode == adios2_mode_read ) then
            call adios2_begin_step_f2c(engine%f2c, &
                                       adios2_step_mode_next_available, 0._4, &
                                       ierr)
        else
            call adios2_begin_step_f2c(engine%f2c, &
                                       adios2_step_mode_append, 0._4, ierr)
        end if

    end subroutine

end module
