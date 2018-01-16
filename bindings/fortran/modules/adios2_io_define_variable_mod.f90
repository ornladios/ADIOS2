!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_io_define_variable_mod.f90 : ADIOS2 Fortran bindings for IO class
!  overloaded (C++ template) function adios2_define_variable
!
!   Created on: Mar 13, 2017
!       Author: William F Godoy godoywf@ornl.gov
!
module adios2_io_define_variable

    use adios2_parameters
    use adios2_functions
    implicit none

    interface adios2_define_variable

        ! Global value
        ! module procedure adios2_define_variable_string
        module procedure adios2_define_variable_real
        module procedure adios2_define_variable_dp
        module procedure adios2_define_variable_complex
        module procedure adios2_define_variable_complex_dp
        module procedure adios2_define_variable_integer1
        module procedure adios2_define_variable_integer2
        module procedure adios2_define_variable_integer4
        module procedure adios2_define_variable_integer8

        ! 1D Array
        module procedure adios2_define_variable_real_1d
        module procedure adios2_define_variable_dp_1d
        module procedure adios2_define_variable_complex_1d
        module procedure adios2_define_variable_complex_dp_1d
        module procedure adios2_define_variable_integer1_1d
        module procedure adios2_define_variable_integer2_1d
        module procedure adios2_define_variable_integer4_1d
        module procedure adios2_define_variable_integer8_1d

        ! 2D Array
        module procedure adios2_define_variable_real_2d
        module procedure adios2_define_variable_dp_2d
        module procedure adios2_define_variable_complex_2d
        module procedure adios2_define_variable_complex_dp_2d
        module procedure adios2_define_variable_integer1_2d
        module procedure adios2_define_variable_integer2_2d
        module procedure adios2_define_variable_integer4_2d
        module procedure adios2_define_variable_integer8_2d

        ! 3D Array
        module procedure adios2_define_variable_real_3d
        module procedure adios2_define_variable_dp_3d
        module procedure adios2_define_variable_complex_3d
        module procedure adios2_define_variable_complex_dp_3d
        module procedure adios2_define_variable_integer1_3d
        module procedure adios2_define_variable_integer2_3d
        module procedure adios2_define_variable_integer4_3d
        module procedure adios2_define_variable_integer8_3d

        ! 4D Array
        module procedure adios2_define_variable_real_4d
        module procedure adios2_define_variable_dp_4d
        module procedure adios2_define_variable_complex_4d
        module procedure adios2_define_variable_complex_dp_4d
        module procedure adios2_define_variable_integer1_4d
        module procedure adios2_define_variable_integer2_4d
        module procedure adios2_define_variable_integer4_4d
        module procedure adios2_define_variable_integer8_4d

        ! 5D Array
        module procedure adios2_define_variable_real_5d
        module procedure adios2_define_variable_dp_5d
        module procedure adios2_define_variable_complex_5d
        module procedure adios2_define_variable_complex_dp_5d
        module procedure adios2_define_variable_integer1_5d
        module procedure adios2_define_variable_integer2_5d
        module procedure adios2_define_variable_integer4_5d
        module procedure adios2_define_variable_integer8_5d

        ! 6D Array
        module procedure adios2_define_variable_real_6d
        module procedure adios2_define_variable_dp_6d
        module procedure adios2_define_variable_complex_6d
        module procedure adios2_define_variable_complex_dp_6d
        module procedure adios2_define_variable_integer1_6d
        module procedure adios2_define_variable_integer2_6d
        module procedure adios2_define_variable_integer4_6d
        module procedure adios2_define_variable_integer8_6d

    end interface

contains

    subroutine adios2_define_variable_real(variable, io, name, data, &
                                           ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        real, intent(in):: data
        integer, intent(out) :: ierr

        call adios2_define_global_variable_f2c(variable, io, &
                                               TRIM(ADJUSTL(name))//char(0), &
                                               adios2_type_real, data, ierr)
    end subroutine

    subroutine adios2_define_variable_dp(variable, io, name, data, &
                                           ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        real(kind=8), intent(in):: data
        integer, intent(out) :: ierr

        call adios2_define_global_variable_f2c(variable, io, &
                                               TRIM(ADJUSTL(name))//char(0), &
                                               adios2_type_dp, data, ierr)
    end subroutine

    subroutine adios2_define_variable_complex(variable, io, name, data, &
                                             ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        complex, intent(in):: data
        integer, intent(out) :: ierr

        call adios2_define_global_variable_f2c(variable, io, &
                                               TRIM(ADJUSTL(name))//char(0), &
                                               adios2_type_complex, data, ierr)
    end subroutine

    subroutine adios2_define_variable_complex_dp(variable, io, name, data, &
                                             ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        complex(kind=8), intent(in):: data
        integer, intent(out) :: ierr

        call adios2_define_global_variable_f2c(variable, io, &
                                               TRIM(ADJUSTL(name))//char(0), &
                                               adios2_type_complex_dp, data, &
                                               ierr)
    end subroutine

    subroutine adios2_define_variable_integer1(variable, io, name, data, &
                                               ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer(kind=1), intent(in):: data
        integer, intent(out) :: ierr

        call adios2_define_global_variable_f2c(variable, io, &
                                               TRIM(ADJUSTL(name))//char(0), &
                                               adios2_type_integer1, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer2(variable, io, name, data, &
                                               ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer(kind=2), intent(in):: data
        integer, intent(out) :: ierr

        call adios2_define_global_variable_f2c(variable, io, &
                                               TRIM(ADJUSTL(name))//char(0), &
                                               adios2_type_integer2, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer4(variable, io, name, data, &
                                               ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer(kind=4), intent(in):: data
        integer, intent(out) :: ierr

        call adios2_define_global_variable_f2c(variable, io, &
                                               TRIM(ADJUSTL(name))//char(0), &
                                               adios2_type_integer4, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer8(variable, io, name, data, &
                                               ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer(kind=8), intent(in):: data
        integer, intent(out) :: ierr

        call adios2_define_global_variable_f2c(variable, io, &
                                               TRIM(ADJUSTL(name))//char(0), &
                                               adios2_type_integer8, data, ierr)
    end subroutine

    ! 1D
    subroutine adios2_define_variable_real_1d(variable, io, name, ndims, &
                                              shape_dims, start_dims, &
                                              count_dims, &
                                              is_constant_dims, data, &
                                              ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        real, dimension(:), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_real, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_dp_1d(variable, io, name, ndims, &
                                            shape_dims, start_dims, &
                                            count_dims, &
                                            is_constant_dims, data, &
                                            ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        real(kind=8), dimension(:), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_dp, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_complex_1d(variable, io, name, ndims, &
                                                 shape_dims, start_dims, &
                                                 count_dims, &
                                                 is_constant_dims, data, &
                                                 ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        complex, dimension(:), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_complex, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_complex_dp_1d(variable, io, name, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, &
                                                    is_constant_dims, data, &
                                                    ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        complex(kind=8), dimension(:), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_complex_dp, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer1_1d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=1), dimension(:), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer1, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer2_1d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=2), dimension(:), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer2, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer4_1d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=4), dimension(:), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer4, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer8_1d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=8), dimension(:), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer8, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine


    !2D
    subroutine adios2_define_variable_real_2d(variable, io, name, ndims, &
                                              shape_dims, start_dims, &
                                              count_dims, &
                                              is_constant_dims, data, &
                                              ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        real, dimension(:, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_real, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_dp_2d(variable, io, name, ndims, &
                                            shape_dims, start_dims, &
                                            count_dims, &
                                            is_constant_dims, data, &
                                            ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        real(kind=8), dimension(:, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_dp, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_complex_2d(variable, io, name, ndims, &
                                                 shape_dims, start_dims, &
                                                 count_dims, &
                                                 is_constant_dims, data, &
                                                 ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        complex, dimension(:, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_complex, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_complex_dp_2d(variable, io, name, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, &
                                                    is_constant_dims, data, &
                                                    ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        complex(kind=8), dimension(:, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_complex_dp, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer1_2d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=1), dimension(:, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer1, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer2_2d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=2), dimension(:, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer2, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer4_2d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=4), dimension(:, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer4, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer8_2d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=8), dimension(:, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer8, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    ! 3D
    subroutine adios2_define_variable_real_3d(variable, io, name, ndims, &
                                              shape_dims, start_dims, &
                                              count_dims, &
                                              is_constant_dims, data, &
                                              ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        real, dimension(:, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_real, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_dp_3d(variable, io, name, ndims, &
                                            shape_dims, start_dims, &
                                            count_dims, &
                                            is_constant_dims, data, &
                                            ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        real(kind=8), dimension(:, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_dp, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_complex_3d(variable, io, name, ndims, &
                                                 shape_dims, start_dims, &
                                                 count_dims, &
                                                 is_constant_dims, data, &
                                                 ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        complex, dimension(:, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_complex, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_complex_dp_3d(variable, io, name, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, &
                                                    is_constant_dims, data, &
                                                    ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        complex(kind=8), dimension(:, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_complex_dp, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer1_3d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=1), dimension(:, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer1, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer2_3d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=2), dimension(:, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer2, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer4_3d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=4), dimension(:, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer4, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer8_3d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=8), dimension(:, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer8, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    ! 4D
    subroutine adios2_define_variable_real_4d(variable, io, name, ndims, &
                                              shape_dims, start_dims, &
                                              count_dims, &
                                              is_constant_dims, data, &
                                              ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        real, dimension(:, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_real, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_dp_4d(variable, io, name, ndims, &
                                            shape_dims, start_dims, &
                                            count_dims, &
                                            is_constant_dims, data, &
                                            ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        real(kind=8), dimension(:, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_dp, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_complex_4d(variable, io, name, ndims, &
                                                 shape_dims, start_dims, &
                                                 count_dims, &
                                                 is_constant_dims, data, &
                                                 ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        complex, dimension(:, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_complex, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_complex_dp_4d(variable, io, name, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, &
                                                    is_constant_dims, data, &
                                                    ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        complex(kind=8), dimension(:, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_complex_dp, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer1_4d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=1), dimension(:, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer1, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer2_4d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=2), dimension(:, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer2, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer4_4d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=4), dimension(:, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer4, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer8_4d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=8), dimension(:, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer8, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    ! 5D
    subroutine adios2_define_variable_real_5d(variable, io, name, ndims, &
                                              shape_dims, start_dims, &
                                              count_dims, &
                                              is_constant_dims, data, &
                                              ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        real, dimension(:, :, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_real, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_dp_5d(variable, io, name, ndims, &
                                            shape_dims, start_dims, &
                                            count_dims, &
                                            is_constant_dims, data, &
                                            ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        real(kind=8), dimension(:, :, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_dp, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_complex_5d(variable, io, name, ndims, &
                                                 shape_dims, start_dims, &
                                                 count_dims, &
                                                 is_constant_dims, data, &
                                                 ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        complex, dimension(:, :, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_complex, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_complex_dp_5d(variable, io, name, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, &
                                                    is_constant_dims, data, &
                                                    ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        complex(kind=8), dimension(:, :, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_complex_dp, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer1_5d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=1), dimension(:, :, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer1, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer2_5d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=2), dimension(:, :, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer2, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer4_5d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=4), dimension(:, :, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer4, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer8_5d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=8), dimension(:, :, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer8, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    ! 6D
    subroutine adios2_define_variable_real_6d(variable, io, name, ndims, &
                                              shape_dims, start_dims, &
                                              count_dims, &
                                              is_constant_dims, data, &
                                              ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        real, dimension(:, :, :, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_real, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_dp_6d(variable, io, name, ndims, &
                                            shape_dims, start_dims, &
                                            count_dims, &
                                            is_constant_dims, data, &
                                            ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        real(kind=8), dimension(:, :, :, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_dp, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_complex_6d(variable, io, name, ndims, &
                                                 shape_dims, start_dims, &
                                                 count_dims, &
                                                 is_constant_dims, data, &
                                                 ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        complex, dimension(:, :, :, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_complex, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_complex_dp_6d(variable, io, name, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, &
                                                    is_constant_dims, data, &
                                                    ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        complex(kind=8), dimension(:, :, :, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_complex_dp, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer1_6d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=1), dimension(:, :, :, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer1, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer2_6d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=2), dimension(:, :, :, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer2, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer4_6d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=4), dimension(:, :, :, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer4, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

    subroutine adios2_define_variable_integer8_6d(variable, io, name, ndims, &
                                                  shape_dims, start_dims, &
                                                  count_dims, &
                                                  is_constant_dims, data, &
                                                  ierr)
        integer(kind=8), intent(out) :: variable
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in) :: ndims
        integer(kind=8), dimension(:), intent(in) :: shape_dims
        integer(kind=8), dimension(:), intent(in) :: start_dims
        integer(kind=8), dimension(:), intent(in) :: count_dims
        logical, intent(in) :: is_constant_dims
        integer(kind=8), dimension(:, :, :, :, :, :), intent(in):: data
        integer, intent(out) :: ierr

        integer is_constant_dims_int
        is_constant_dims_int = adios2_LogicalToInt(is_constant_dims)

        call adios2_define_variable_f2c(variable, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer8, ndims, &
                                        shape_dims, start_dims, count_dims, &
                                        is_constant_dims_int, data, ierr)
    end subroutine

end module
