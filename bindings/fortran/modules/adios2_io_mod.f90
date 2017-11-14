!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_io.f90 : ADIOS2 Fortran bindings for IO class
!
!   Created on: Mar 13, 2017
!       Author: William F Godoy godoywf@ornl.gov
!
module adios2_io

    use adios2_io_open
    use adios2_functions
    implicit none

contains

    subroutine adios2_set_parameter(io, key, value, ierr)
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: key
        character*(*), intent(in) :: value
        integer, intent(out) :: ierr

        call adios2_set_parameter_f2c( io, TRIM(ADJUSTL(key))//char(0), &
            & TRIM(ADJUSTL(value))//char(0), ierr )

    end subroutine


   subroutine adios2_add_transport(transport_index, io, transport_type, ierr)
        integer, intent(out):: transport_index
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: transport_type
        integer, intent(out) :: ierr

        call adios2_add_transport_f2c( transport_index, io, &
            & TRIM(ADJUSTL(transport_type))//char(0), ierr)

    end subroutine


    subroutine adios2_set_transport_parameter(io, transport_index, key, value, &
        & ierr)
        integer(kind=8), intent(in):: io
        integer, intent(in):: transport_index
        character*(*), intent(in) :: key
        character*(*), intent(in) :: value
        integer, intent(out):: ierr

        call adios2_set_transport_parameter_f2c(io, transport_index, &
          & TRIM(ADJUSTL(key))//char(0), TRIM(ADJUSTL(value))//char(0), &
          & ierr)

    end subroutine


    subroutine adios2_define_variable(variable, io, variable_name, &
        & adios2_type, ndims, shape_dims, start_dims, count_dims, &
        & adios2_constant_dims, ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: variable_name
        integer, intent(in) :: adios2_type
        integer, intent(in) :: ndims
        integer, dimension(:), intent(in) :: shape_dims
        integer, dimension(:), intent(in) :: start_dims
        integer, dimension(:), intent(in) :: count_dims
        logical, intent(in) :: adios2_constant_dims
        integer, intent(out) :: ierr

        !local
        integer constant_dims

        constant_dims = adios2_LogicalToInt(adios2_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
            & TRIM(ADJUSTL(variable_name))//char(0), adios2_type, ndims, &
            & shape_dims, start_dims, count_dims, constant_dims, ierr)

    end subroutine


end module
