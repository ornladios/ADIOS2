! SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
!
! SPDX-License-Identifier: Apache-2.0


module adios2_operator_mod
    use adios2_parameters_mod
    implicit none

contains

    subroutine adios2_operator_type(type, op, ierr)
        character(len=:), allocatable, intent(out) :: type
        type(adios2_operator), intent(in) :: op
        integer, intent(out) :: ierr
        external adios2_operator_type_f2c
        external adios2_operator_type_length_f2c

        !local
        integer :: length

        if (allocated(type)) deallocate (type)

        call adios2_operator_type_length_f2c(length, op%f2c, ierr)
        if (ierr == 0) then
            allocate (character(length) :: type)
            call adios2_operator_type_f2c(type, op%f2c, ierr)
        end if

    end subroutine

end module
