!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_variable_mod.f90 : ADIOS2 Fortran bindings for Variable class
!
!   Created on: Mar 13, 2017
!       Author: William F Godoy godoywf@ornl.gov
!

module adios2_variable_mod
    use adios2_functions_mod
    implicit none

contains

    subroutine adios2_variable_name(variable, name, ierr)
        type(adios2_variable), intent(in) :: variable
        character(len=:), allocatable, intent(out) :: name
        integer, intent(out) :: ierr

        character(len=1024) :: c_name
        integer :: length

        call adios2_variable_name_f2c(variable%f2c, c_name, length, ierr)
        call adios2_StringC2F(c_name, length, name)

    end subroutine

    subroutine adios2_variable_type(variable, type, ierr)
        type(adios2_variable), intent(in) :: variable
        integer, intent(out) :: type
        integer, intent(out) :: ierr

        integer :: c_type

        call adios2_variable_type_f2c(variable%f2c, c_type, ierr)
        call adios2_TypeC2F(c_type, type)

    end subroutine

    subroutine adios2_variable_ndims(variable, ndims, ierr)
        type(adios2_variable), intent(in) :: variable
        integer, intent(out) :: ndims
        integer, intent(out) :: ierr

        call adios2_variable_ndims_f2c(variable%f2c, ndims, ierr)
    end subroutine

    subroutine adios2_variable_shape(variable, ndims, shape_dims, ierr)
        type(adios2_variable), intent(in) :: variable
        integer, intent(out) :: ndims
        integer(kind=8), dimension(:), allocatable, intent(out) :: shape_dims
        integer, intent(out) :: ierr

        call adios2_variable_ndims_f2c(variable%f2c, ndims, ierr)
        allocate (shape_dims(ndims))
        call adios2_variable_shape_f2c(variable%f2c, shape_dims, ierr)

    end subroutine

    subroutine adios2_variable_available_steps_start(variable, steps_start, &
                                                     ierr)
        type(adios2_variable), intent(in) :: variable
        integer(kind=8), intent(out) :: steps_start
        integer, intent(out) :: ierr

        call adios2_variable_available_steps_start_f2c(variable%f2c, &
                                                       steps_start, ierr)
    end subroutine

    subroutine adios2_variable_available_steps_count(variable, steps_count, &
                                                     ierr)
        type(adios2_variable), intent(in) :: variable
        integer(kind=8), intent(out) :: steps_count
        integer, intent(out) :: ierr

        call adios2_variable_available_steps_count_f2c(variable%f2c, &
                                                       steps_count, ierr)
    end subroutine

    subroutine adios2_set_shape(variable, ndims, shape_dims, ierr)
        type(adios2_variable), intent(in) :: variable
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer, intent(out) :: ierr

        call adios2_set_shape_f2c(variable%f2c, ndims, shape_dims, ierr)
    end subroutine

    subroutine adios2_set_selection(variable, ndims, start_dims, count_dims, &
                                    ierr)
        type(adios2_variable), intent(in) :: variable
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        integer, intent(out) :: ierr

        call adios2_set_selection_f2c(variable%f2c, ndims, start_dims, &
                                      count_dims, ierr)
    end subroutine

    subroutine adios2_set_step_selection(variable, step_start, step_count, ierr)
        type(adios2_variable), intent(in) :: variable
        integer(kind=8), intent(in) :: step_start
        integer(kind=8), intent(in) :: step_count
        integer, intent(out) :: ierr

        call adios2_set_step_selection_f2c(variable%f2c, step_start, &
                                           step_count, ierr)
    end subroutine


    subroutine adios2_variable_check_type(variable, adios2_type, hint, ierr)
        type(adios2_variable), intent(in):: variable
        integer, intent(in):: adios2_type
        character*(*), intent(in):: hint
        integer, intent(out):: ierr
        !local
        integer :: variable_type
        character(len=:), allocatable :: variable_name

        call adios2_variable_type(variable, variable_type, ierr)

        if( variable_type /= adios2_type ) then
            call adios2_variable_name(variable, variable_name, ierr)
            write(0,*) 'ERROR: adios2 variable ', variable_name, &
                       ' type mismatch, in call to adios2_', hint
            ierr = -1
        end if

    end subroutine

end module
