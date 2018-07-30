!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_mod.f90 : ADIOS2 Fortran bindings central module
!
!   Created on: Mar 13, 2017
!       Author: William F Godoy godoywf@ornl.gov
!

module adios2_parameters_mod
    implicit none

    ! Debug mode
    logical, parameter :: adios2_debug_mode_on = .true.
    logical, parameter :: adios2_debug_mode_off = .false.

    ! Types
    integer, parameter :: adios2_type_unknown = -1
    integer, parameter :: adios2_type_character = 0
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

    ! is_constant_dims
    logical, parameter :: adios2_constant_dims = .true.
    logical, parameter :: adios2_variable_dims = .false.

    ! Variable Found or not found, ierr value
    integer, parameter :: adios2_not_found = -1
    integer, parameter :: adios2_found = 0

    ! Mode
    integer, parameter :: adios2_mode_undefined = 0
    integer, parameter :: adios2_mode_write = 1
    integer, parameter :: adios2_mode_read = 2
    integer, parameter :: adios2_mode_append = 3
    integer, parameter :: adios2_mode_deferred = 4
    integer, parameter :: adios2_mode_sync = 5

    ! Step Mode
    integer, parameter :: adios2_step_mode_append = 0
    integer, parameter :: adios2_step_mode_update = 1
    integer, parameter :: adios2_step_mode_next_available = 2
    integer, parameter :: adios2_step_mode_latest_available = 3

    ! Step Status
    integer, parameter :: adios2_step_status_other_error = -1
    integer, parameter :: adios2_step_status_ok = 0
    integer, parameter :: adios2_step_status_not_ready = 1
    integer, parameter :: adios2_step_status_end_of_stream = 2

    !> Fixed size for string array, used in variables and attributes,
    !! must be less or equal than C equivalent in adios2_c_types.h
    integer, parameter :: adios2_string_array_element_max_size = 4096

    integer(kind=8), parameter, dimension(1) :: adios2_null_dims = (/-1/)

    ! Debug mode
    logical, parameter :: adios2_advance_yes = .true.
    logical, parameter :: adios2_advance_no  = .false.

    ! Low level API handlers
    type adios2_adios
        integer(kind=8):: f2c = 0_8
        logical :: valid = .false.
    end type

    type adios2_io
        integer(kind=8):: f2c = 0_8
        logical :: valid = .false.
        character(len=15):: engine_type = 'BPFile'
    end type

    type adios2_variable
        integer(kind=8):: f2c = 0_8
        logical :: valid = .false.
        character(len=1024):: name = ''
        integer :: type = -1
        integer :: ndims = -1
    end type

    type adios2_attribute
        integer(kind=8):: f2c = 0_8
        logical :: valid = .false.
        character(len=1024):: name = ''
        integer :: type = -1
        integer :: length = 0
    end type

    type adios2_engine
        integer(kind=8):: f2c = 0_8
        logical :: valid = .false.
        character(len=1024):: name = ''
        character(len=15):: type = ''
        integer :: mode = adios2_mode_undefined
    end type

    type adios2_operator
        integer(kind=8):: f2c = 0_8
        logical :: valid = .false.
        character(len=1024):: name = ''
        character(len=1024):: type = ''
    end type


    ! High-level API
    type adios2_file
        integer(kind=8):: f2c = 0_8
        logical :: valid = .false.
    end type

end module
