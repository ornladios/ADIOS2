!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_mod.f90 : ADIOS2 Fortran bindings central module
!
!   Created on: Mar 13, 2017
!       Author: William F Godoy godoywf@ornl.gov
!
module adios2_parameters
    implicit none

    ! Debug mode
    logical, parameter :: adios2_debug_mode_on = .true.
    logical, parameter :: adios2_debug_mode_off = .false.

    ! Types
    integer, parameter :: adios2_type_unknown = -1
    integer, parameter :: adios2_type_character = 0
    integer, parameter :: adios2_type_integer = 1
    integer, parameter :: adios2_type_real = 2
    integer, parameter :: adios2_type_dp = 3
    integer, parameter :: adios2_type_complex = 4
    integer, parameter :: adios2_type_complex_dp = 5

    integer, parameter :: adios2_type_integer1 = 6
    integer, parameter :: adios2_type_integer2 = 7
    integer, parameter :: adios2_type_integer4 = 8
    integer, parameter :: adios2_type_integer8 = 9

    integer, parameter :: adios2_type_string = 10
    integer, parameter :: adios2_type_string_array = 11

    ! Constant dims
    logical, parameter :: adios2_constant_dims_true = .true.
    logical, parameter :: adios2_constant_dims_false = .false.

    ! Mode
    integer, parameter :: adios2_mode_undefined = 0
    integer, parameter :: adios2_mode_write = 1
    integer, parameter :: adios2_mode_read = 2
    integer, parameter :: adios2_mode_append = 3

end module
