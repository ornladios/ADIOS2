!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_io_mod.f90 : ADIOS2 Fortran bindings for IO class
!
!   Created on: Mar 13, 2017
!       Author: William F Godoy godoywf@ornl.gov
!
module adios2_io

    use adios2_io_open
    use adios2_io_define_variable
    implicit none

contains

    subroutine adios2_set_parameter(io, key, value, ierr)
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: key
        character*(*), intent(in) :: value
        integer, intent(out) :: ierr

        call adios2_set_parameter_f2c(io, TRIM(ADJUSTL(key))//char(0), &
                                      TRIM(ADJUSTL(value))//char(0), ierr)

    end subroutine


    subroutine adios2_add_transport(transport_index, io, type, ierr)
        integer, intent(out):: transport_index
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: type
        integer, intent(out) :: ierr

        call adios2_add_transport_f2c(transport_index, io, &
                                      TRIM(ADJUSTL(type))//char(0), ierr)


    end subroutine


    subroutine adios2_set_transport_parameter(io, transport_index, key, value, &
                                              ierr)
        integer(kind=8), intent(in):: io
        integer, intent(in):: transport_index
        character*(*), intent(in) :: key
        character*(*), intent(in) :: value
        integer, intent(out):: ierr

        call adios2_set_transport_parameter_f2c(io, transport_index, &
                                                TRIM(ADJUSTL(key))//char(0), &
                                                TRIM(ADJUSTL(value))//char(0), &
                                                ierr)

    end subroutine

    subroutine adios2_inquire_variable(variable, io, name, ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(out) :: ierr

        call adios2_inquire_variable_f2c(variable, io, &
                                         TRIM(ADJUSTL(name))//char(0), ierr)

    end subroutine

end module
