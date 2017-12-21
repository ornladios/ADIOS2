!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_io_define_attribute_mod.f90 : ADIOS2 Fortran bindings for IO class
!  overloaded (C++ template) function adios2_define_attribute
!
!   Created on: Dec 15, 2017
!       Author: William F Godoy godoywf@ornl.gov
!
module adios2_io_define_attribute

    use adios2_parameters
    implicit none

    interface adios2_define_attribute

        ! Global value
        module procedure adios2_define_attribute_string
        module procedure adios2_define_attribute_integer
        module procedure adios2_define_attribute_real
        module procedure adios2_define_attribute_dp
        module procedure adios2_define_attribute_integer1
        module procedure adios2_define_attribute_integer2
        module procedure adios2_define_attribute_integer8

        ! 1D Array
        module procedure adios2_define_attribute_string_1d
        module procedure adios2_define_attribute_integer_1d
        module procedure adios2_define_attribute_real_1d
        module procedure adios2_define_attribute_dp_1d
        module procedure adios2_define_attribute_integer1_1d
        module procedure adios2_define_attribute_integer2_1d
        module procedure adios2_define_attribute_integer8_1d

    end interface

contains

    subroutine adios2_define_attribute_string(attribute, io, name, data, ierr)
        integer(kind=8), intent(out) :: attribute
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        character*(*), intent(in):: data
        integer, intent(out) :: ierr

        call adios2_define_attribute_f2c(attribute, io, &
                                         TRIM(ADJUSTL(name))//char(0), &
                                         adios2_type_string, &
                                         TRIM(ADJUSTL(data))//char(0), 1, ierr)
    end subroutine

    subroutine adios2_define_attribute_integer(attribute, io, name, data, ierr)
        integer(kind=8), intent(out) :: attribute
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(in):: data
        integer, intent(out) :: ierr

        call adios2_define_attribute_f2c(attribute, io, &
                                         TRIM(ADJUSTL(name))//char(0), &
                                         adios2_type_integer, data, 1, ierr)
    end subroutine

    subroutine adios2_define_attribute_real(attribute, io, name, data, ierr)

        integer(kind=8), intent(out) :: attribute
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        real, intent(in):: data
        integer, intent(out) :: ierr

        call adios2_define_attribute_f2c(attribute, io, &
                                         TRIM(ADJUSTL(name))//char(0), &
                                         adios2_type_real, data, 1, ierr)
    end subroutine

    subroutine adios2_define_attribute_dp(attribute, io, name, data, ierr)
        integer(kind=8), intent(out) :: attribute
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        real(kind=8), intent(in):: data
        integer, intent(out) :: ierr

        call adios2_define_attribute_f2c(attribute, io, &
                                         TRIM(ADJUSTL(name))//char(0), &
                                         adios2_type_dp, data, 1, ierr)
    end subroutine

    subroutine adios2_define_attribute_integer1(attribute, io, name, data, ierr)

        integer(kind=8), intent(out) :: attribute
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer(kind=1), intent(in):: data
        integer, intent(out) :: ierr

        call adios2_define_attribute_f2c(attribute, io, &
                                         TRIM(ADJUSTL(name))//char(0), &
                                         adios2_type_integer1, data, 1, ierr)
    end subroutine

    subroutine adios2_define_attribute_integer2(attribute, io, name, data, ierr)

        integer(kind=8), intent(out) :: attribute
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer(kind=2), intent(in):: data
        integer, intent(out) :: ierr

        call adios2_define_attribute_f2c(attribute, io, &
                                         TRIM(ADJUSTL(name))//char(0), &
                                         adios2_type_integer2, data, 1, ierr)
    end subroutine


    subroutine adios2_define_attribute_integer8(attribute, io, name, data, ierr)

        integer(kind=8), intent(out) :: attribute
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer(kind=8), intent(in):: data
        integer, intent(out) :: ierr

        call adios2_define_attribute_f2c(attribute, io, &
                                         TRIM(ADJUSTL(name))//char(0), &
                                         adios2_type_integer8, data, 1, ierr)
    end subroutine

    subroutine adios2_define_attribute_string_1d(attribute, io, name, &
                                                 data, elements, ierr)
        integer(kind=8), intent(out) :: attribute
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        character*(*), dimension(:), intent(in):: data
        integer, intent(in) :: elements
        integer, intent(out) :: ierr

        ! local data with zero terminated character
        character(len=adios2_string_array_element_max_size), &
        dimension(elements):: data_null_terminated

        integer :: i

        do i=1,elements
            data_null_terminated(i) = TRIM(ADJUSTL(data(i)))//char(0)
        end do

        call adios2_define_attribute_f2c(attribute, io, &
                                         TRIM(ADJUSTL(name))//char(0), &
                                         adios2_type_string_array, &
                                         data_null_terminated, elements, &
                                         ierr)

    end subroutine


    subroutine adios2_define_attribute_integer_1d(attribute, io, name, &
                                                  data, elements, ierr )
        integer(kind=8), intent(out) :: attribute
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer, dimension(:), intent(in):: data
        integer, intent(in) :: elements
        integer, intent(out) :: ierr

        call adios2_define_attribute_f2c(attribute, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer, data, elements, &
                                        ierr)
    end subroutine

    subroutine adios2_define_attribute_real_1d(attribute, io, name, data, &
                                               elements, ierr)
        integer(kind=8), intent(out) :: attribute
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        real, dimension(:), intent(in):: data
        integer, intent(in) :: elements
        integer, intent(out) :: ierr

        call adios2_define_attribute_f2c(attribute, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_real, data, elements, ierr)
    end subroutine

    subroutine adios2_define_attribute_dp_1d(attribute, io, name, data, &
                                             elements, ierr)
        integer(kind=8), intent(out) :: attribute
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        real(kind=8), dimension(:), intent(in):: data
        integer, intent(in) :: elements
        integer, intent(out) :: ierr

        call adios2_define_attribute_f2c(attribute, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_dp, data, elements, ierr)
    end subroutine


    subroutine adios2_define_attribute_integer1_1d(attribute, io, name, &
                                                   data, elements, ierr)

        integer(kind=8), intent(out) :: attribute
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer(kind=1), dimension(:), intent(in):: data
        integer, intent(in) :: elements
        integer, intent(out) :: ierr

        call adios2_define_attribute_f2c(attribute, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer1, &
                                        data, elements, ierr)
    end subroutine

    subroutine adios2_define_attribute_integer2_1d(attribute, io, name, &
                                                   data, elements, ierr)

        integer(kind=8), intent(out) :: attribute
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer(kind=2), dimension(:), intent(in):: data
        integer, intent(in) :: elements
        integer, intent(out) :: ierr

        call adios2_define_attribute_f2c(attribute, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer2, &
                                        data, elements, ierr)
    end subroutine

    subroutine adios2_define_attribute_integer8_1d(attribute, io, name, &
                                                   data, elements, ierr)

        integer(kind=8), intent(out) :: attribute
        integer(kind=8), intent(in) :: io
        character*(*), intent(in) :: name
        integer(kind=8), dimension(:), intent(in):: data
        integer, intent(in) :: elements
        integer, intent(out) :: ierr

        call adios2_define_attribute_f2c(attribute, io, &
                                        TRIM(ADJUSTL(name))//char(0), &
                                        adios2_type_integer8, &
                                        data, elements, ierr)
    end subroutine

end module
