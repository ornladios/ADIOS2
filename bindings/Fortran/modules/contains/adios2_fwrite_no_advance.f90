!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_fwrite_no_advance.f90 : ADIOS2 Fortran bindings implementation for
!                                     high-level API Write functions
!                                     no adios2_advance flag
!
!   Created on: Mar 1st, 2018
!       Author: William F Godoy godoywf@ornl.gov
!

! Single value
subroutine adios2_fwrite_no_advance_real(unit, name, data, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    real, intent(in):: data
    integer, intent(out) :: ierr

    call adios2_fwrite_value_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                                     adios2_type_real, data, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_dp(unit, name, data, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    real(kind=8), intent(in):: data
    integer, intent(out) :: ierr

    call adios2_fwrite_value_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                                     adios2_type_dp, data, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_complex(unit, name, data, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    complex, intent(in):: data
    integer, intent(out) :: ierr

    call adios2_fwrite_value_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                                     adios2_type_complex, data, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_complex_dp(unit, name, data, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    complex(kind=8), intent(in):: data
    integer, intent(out) :: ierr

    call adios2_fwrite_value_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                                     adios2_type_complex_dp, data, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer1(unit, name, data, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=1), intent(in):: data
    integer, intent(out) :: ierr

    call adios2_fwrite_value_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                                     adios2_type_integer1, data, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer2(unit, name, data, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=2), intent(in):: data
    integer, intent(out) :: ierr

    call adios2_fwrite_value_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                                     adios2_type_integer2, data, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer4(unit, name, data, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=4), intent(in):: data
    integer, intent(out) :: ierr

    call adios2_fwrite_value_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                                     adios2_type_integer4, data, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer8(unit, name, data, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=8), intent(in):: data
    integer, intent(out) :: ierr

    call adios2_fwrite_value_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                                     adios2_type_integer8, data, 0, ierr)
end subroutine

! 1D arrays
subroutine adios2_fwrite_no_advance_real_1d(unit, name, data, ndims, &
                                                shape_dims, start_dims, &
                                                count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    real, dimension(:), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_real, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_dp_1d(unit, name, data, ndims, &
                                              shape_dims, start_dims, &
                                              count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    real(kind=8), dimension(:), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_dp, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_complex_1d(unit, name, data, ndims, &
                                                   shape_dims, start_dims, &
                                                   count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    complex, dimension(:), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_complex, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_complex_dp_1d(unit, name, data, ndims, &
                                                      shape_dims, start_dims, &
                                                      count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    complex(kind=8), dimension(:), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_complex_dp, data, ndims, &
                               shape_dims, start_dims, count_dims, 0, ierr)

end subroutine

subroutine adios2_fwrite_no_advance_integer1_1d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=1), dimension(:), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer1, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer2_1d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=2), dimension(:), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer2, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer4_1d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=4), dimension(:), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer4, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer8_1d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=8), dimension(:), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer8, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

! 2D arrays
subroutine adios2_fwrite_no_advance_real_2d(unit, name, data, ndims, &
                                                shape_dims, start_dims, &
                                                count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    real, dimension(:, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_real, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_dp_2d(unit, name, data, ndims, &
                                              shape_dims, start_dims, &
                                              count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    real(kind=8), dimension(:, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_dp, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_complex_2d(unit, name, data, ndims, &
                                                   shape_dims, start_dims, &
                                                   count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    complex, dimension(:, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_complex, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_complex_dp_2d(unit, name, data, ndims, &
                                                      shape_dims, start_dims, &
                                                      count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    complex(kind=8), dimension(:, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_complex_dp, data, ndims, &
                               shape_dims, start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer1_2d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=1), dimension(:, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer1, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer2_2d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=2), dimension(:, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer2, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer4_2d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=4), dimension(:, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer4, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer8_2d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=8), dimension(:, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer8, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

! 3D arrays
subroutine adios2_fwrite_no_advance_real_3d(unit, name, data, ndims, &
                                                shape_dims, start_dims, &
                                                count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    real, dimension(:, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_real, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_dp_3d(unit, name, data, ndims, &
                                              shape_dims, start_dims, &
                                              count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    real(kind=8), dimension(:, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_dp, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_complex_3d(unit, name, data, ndims, &
                                                   shape_dims, start_dims, &
                                                   count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    complex, dimension(:, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_complex, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_complex_dp_3d(unit, name, data, ndims, &
                                                      shape_dims, start_dims, &
                                                      count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    complex(kind=8), dimension(:, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_complex_dp, data, ndims, &
                               shape_dims, start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer1_3d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=1), dimension(:, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer1, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer2_3d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=2), dimension(:, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer2, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer4_3d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=4), dimension(:, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer4, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer8_3d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=8), dimension(:, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer8, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

! 4D arrays
subroutine adios2_fwrite_no_advance_real_4d(unit, name, data, ndims, &
                                                shape_dims, start_dims, &
                                                count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    real, dimension(:, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_real, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_dp_4d(unit, name, data, ndims, &
                                              shape_dims, start_dims, &
                                              count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    real(kind=8), dimension(:, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_dp, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_complex_4d(unit, name, data, ndims, &
                                                   shape_dims, start_dims, &
                                                   count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    complex, dimension(:, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_complex, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_complex_dp_4d(unit, name, data, ndims, &
                                                      shape_dims, start_dims, &
                                                      count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    complex(kind=8), dimension(:, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_complex_dp, data, ndims, &
                               shape_dims, start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer1_4d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=1), dimension(:, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer1, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer2_4d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=2), dimension(:, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer2, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer4_4d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=4), dimension(:, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer4, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer8_4d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=8), dimension(:, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer8, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

! 5D arrays
subroutine adios2_fwrite_no_advance_real_5d(unit, name, data, ndims, &
                                                shape_dims, start_dims, &
                                                count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    real, dimension(:, :, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_real, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_dp_5d(unit, name, data, ndims, &
                                              shape_dims, start_dims, &
                                              count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    real(kind=8), dimension(:, :, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_dp, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_complex_5d(unit, name, data, ndims, &
                                                   shape_dims, start_dims, &
                                                   count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    complex, dimension(:, :, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_complex, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_complex_dp_5d(unit, name, data, ndims, &
                                                      shape_dims, start_dims, &
                                                      count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    complex(kind=8), dimension(:, :, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_complex_dp, data, ndims, &
                               shape_dims, start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer1_5d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=1), dimension(:, :, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer1, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer2_5d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=2), dimension(:, :, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer2, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer4_5d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=4), dimension(:, :, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer4, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer8_5d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=8), dimension(:, :, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer8, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

! 6D arrays
subroutine adios2_fwrite_no_advance_real_6d(unit, name, data, ndims, &
                                                shape_dims, start_dims, &
                                                count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    real, dimension(:, :, :, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_real, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_dp_6d(unit, name, data, ndims, &
                                              shape_dims, start_dims, &
                                              count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    real(kind=8), dimension(:, :, :, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_dp, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_complex_6d(unit, name, data, ndims, &
                                                   shape_dims, start_dims, &
                                                   count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    complex, dimension(:, :, :, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_complex, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_complex_dp_6d(unit, name, data, ndims, &
                                                      shape_dims, start_dims, &
                                                      count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    complex(kind=8), dimension(:, :, :, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_complex_dp, data, ndims, &
                               shape_dims, start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer1_6d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=1), dimension(:, :, :, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer1, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer2_6d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=2), dimension(:, :, :, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer2, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer4_6d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=4), dimension(:, :, :, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer4, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

subroutine adios2_fwrite_no_advance_integer8_6d(unit, name, data, ndims, &
                                                    shape_dims, start_dims, &
                                                    count_dims, ierr)
    type(adios2_file), intent(in):: unit
    character*(*), intent(in) :: name
    integer(kind=8), dimension(:, :, :, :, :, :), intent(in):: data
    integer, intent(in) :: ndims
    integer(kind=8), dimension(:), intent(in) :: shape_dims
    integer(kind=8), dimension(:), intent(in) :: start_dims
    integer(kind=8), dimension(:), intent(in) :: count_dims
    integer, intent(out) :: ierr

    call adios2_fwrite_f2c(unit%f2c, TRIM(ADJUSTL(name))//char(0), &
                               adios2_type_integer8, data, ndims, shape_dims, &
                               start_dims, count_dims, 0, ierr)
end subroutine

