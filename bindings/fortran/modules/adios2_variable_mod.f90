!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_variable_mod.f90 : ADIOS2 Fortran bindings for Variable class
!
!   Created on: Mar 13, 2017
!       Author: William F Godoy godoywf@ornl.gov
!

module adios2_variable
    use adios2_functions
    implicit none

contains

    !!<
    !! @param variable
    subroutine adios2_variable_name(variable, name, ierr)
        integer(kind=8), intent(out) :: variable
        character(len=:), allocatable, intent(out) :: name
        integer, intent(out) :: ierr

        character(len=1024) :: c_name
        integer :: length, i

        call adios2_variable_name_f2c(variable, c_name, length, ierr)
        call adios2_StringC2F(c_name, length, name)

    end subroutine

    subroutine adios2_variable_type(variable, type, ierr)
        integer(kind=8), intent(out) :: variable
        integer, intent(out) :: type
        integer, intent(out) :: ierr

        integer :: c_type

        call adios2_variable_type_f2c(variable, c_type, ierr)
        call adios2_TypeC2F(c_type, type)

    end subroutine

    subroutine adios2_set_shape(variable, ndims, shape_dims, ierr)
        integer(kind=8), intent(out) :: variable
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer, intent(out) :: ierr

        call adios2_set_shape_f2c(variable, ndims, shape_dims, ierr)
    end subroutine

    subroutine adios2_set_selection(variable, ndims, start_dims, count_dims, &
        & ierr)
        integer(kind=8), intent(out) :: variable
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        integer, intent(out) :: ierr

        call adios2_set_selection_f2c(variable, ndims, start_dims, count_dims, &
                                      ierr)

    end subroutine

    subroutine adios2_set_step_selection(variable, start_step, count_step, ierr)
        integer(kind=8), intent(out) :: variable
        integer, dimension(:), intent(in) :: start_step
        integer, dimension(:), intent(in) :: count_step
        integer, intent(out) :: ierr

        call adios2_set_step_selection_f2c(variable, start_step, count_step, &
                                           ierr)

    end subroutine

end module
