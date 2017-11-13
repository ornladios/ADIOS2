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
    implicit none

contains

    subroutine adios2_set_selection(variable, ndims, start_dims, count_dims, &
        & ierr)
        integer(kind=8), intent(out) :: variable
        integer, intent(in) :: ndims
        integer, dimension(:), intent(in) :: start_dims
        integer, dimension(:), intent(in) :: count_dims
        integer, intent(out) :: ierr

        call adios2_set_selection_f2c(variable, ndims, start_dims, &
            & count_dims, ierr)

    end subroutine

end module
