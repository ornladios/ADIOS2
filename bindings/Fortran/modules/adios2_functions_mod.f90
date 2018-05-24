!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_function_mod.f90 : Fortran bindings internal functions
!
!   Created on: September 28, 2017
!       Author: William F Godoy godoywf@ornl.gov
!

module adios2_functions_mod
    use adios2_parameters_mod
    use adios2_functions_allocate_mod
    implicit none

contains

    integer function adios2_LogicalToInt(logical_value)
        logical, value, intent(in) :: logical_value

        adios2_LogicalToInt = 0
        if (logical_value) then
            adios2_LogicalToInt = 1
        end if

    end function

    subroutine adios2_StringC2F(c_string, length, f_string)
        character*(*), intent(in) :: c_string
        integer, intent(in) :: length
        character(len=:), allocatable, intent(out) :: f_string

        if (allocated(f_string)) deallocate (f_string)
        if (length > 0) then
            allocate (character(length) :: f_string)
            f_string = c_string(1:length)
        end if

    end subroutine

    subroutine adios2_TypeC2F(c_type, f_type)
        integer, intent(in) :: c_type
        integer, intent(out) :: f_type

        integer :: i

        f_type = adios2_type_unknown

        do i = -1, 11
            if (c_type == i) then
                f_type = c_type
                exit
            end if
        end do

    end subroutine

end module
