!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_io_mod.f90 : ADIOS2 Fortran bindings for IO class
!
!   Created on: Mar 13, 2017
!       Author: William F Godoy godoywf@ornl.gov
!

module adios2_io_mod
    use adios2_io_open_mod
    use adios2_io_define_variable_mod
    use adios2_io_define_attribute_mod
    use adios2_variable_mod
    implicit none

contains

    subroutine adios2_io_engine_type(type, io, ierr)
        character(len=:), allocatable, intent(out) :: type
        type(adios2_io), intent(in) :: io
        integer, intent(out) :: ierr

        !local
        integer :: length

        if (allocated(type)) deallocate (type)

        call adios2_io_engine_type_length_f2c(length, io%f2c, ierr)
        if (ierr == 0) then
            allocate (character(length) :: type)
            call adios2_io_engine_type_f2c(type, io%f2c, ierr)
        end if

    end subroutine

    subroutine adios2_set_engine(io, engine_type, ierr)
        type(adios2_io), intent(inout) :: io
        character*(*), intent(in) :: engine_type
        integer, intent(out) :: ierr

        call adios2_set_engine_f2c(io%f2c, &
                                   TRIM(ADJUSTL(engine_type))//char(0), ierr)

        if( ierr == 0 ) io%engine_type = engine_type

    end subroutine

    subroutine adios2_set_parameter(io, key, value, ierr)
        type(adios2_io), intent(in) :: io
        character*(*), intent(in) :: key
        character*(*), intent(in) :: value
        integer, intent(out) :: ierr

        call adios2_set_parameter_f2c(io%f2c, TRIM(ADJUSTL(key))//char(0), &
                                      TRIM(ADJUSTL(value))//char(0), ierr)
    end subroutine

    subroutine adios2_add_transport(transport_index, io, type, ierr)
        integer, intent(out):: transport_index
        type(adios2_io), intent(in) :: io
        character*(*), intent(in) :: type
        integer, intent(out) :: ierr

        call adios2_add_transport_f2c(transport_index, io%f2c, &
                                      TRIM(ADJUSTL(type))//char(0), ierr)

    end subroutine

    subroutine adios2_set_transport_parameter(io, transport_index, key, value, &
                                              ierr)
        type(adios2_io), intent(in):: io
        integer, intent(in):: transport_index
        character*(*), intent(in) :: key
        character*(*), intent(in) :: value
        integer, intent(out):: ierr

        call adios2_set_transport_parameter_f2c(io%f2c, transport_index, &
                                                TRIM(ADJUSTL(key))//char(0), &
                                                TRIM(ADJUSTL(value))//char(0), &
                                                ierr)
    end subroutine

    subroutine adios2_inquire_variable(variable, io, name, ierr)
        type(adios2_variable), intent(out) :: variable
        type(adios2_io), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(out) :: ierr

        call adios2_inquire_variable_f2c(variable%f2c, io%f2c, &
                                         TRIM(ADJUSTL(name))//char(0), ierr)

        if(variable%f2c > 0_8) then
            variable%valid = .true.
            variable%name = name
            call adios2_variable_type(variable%type, variable, ierr)
            call adios2_variable_ndims(variable%ndims, variable, ierr)
        else
            variable%valid = .false.
            variable%name = ''
            variable%type = adios2_type_unknown
            variable%ndims = -1
        end if

    end subroutine

    subroutine adios2_remove_variable(result, io, name, ierr)
        type(adios2_io), intent(in) :: io
        character*(*), intent(in) :: name
        logical, intent(out) :: result
        integer, intent(out) :: ierr
        ! Local
        type(adios2_variable):: variable
        integer:: resultInt

        call adios2_inquire_variable(variable, io, name, ierr)
        if( variable%valid ) then
            call adios2_remove_variable_f2c(resultInt, io%f2c, &
                                            TRIM(ADJUSTL(name))//char(0), ierr)
            if( resultInt == 1) then
                result = .true.
            else
                result = .false.
            end if
        end if

    end subroutine


    subroutine adios2_remove_all_variables(io, ierr)
        type(adios2_io), intent(in) :: io
        integer, intent(out) :: ierr

        call adios2_remove_all_variables_f2c(io%f2c, ierr)

    end subroutine


    subroutine adios2_inquire_attribute(attribute, io, name, ierr)
        type(adios2_attribute), intent(out) :: attribute
        type(adios2_io), intent(in) :: io
        character*(*), intent(in) :: name
        integer, intent(out) :: ierr
        !local
        integer:: is_valueInt

        call adios2_inquire_attribute_f2c(attribute%f2c, io%f2c, &
                                          TRIM(ADJUSTL(name))//char(0), ierr)

        if(attribute%f2c > 0_8) then
            attribute%valid = .true.
            attribute%name = name
            call adios2_attribute_type_f2c(attribute%type, attribute%f2c, ierr)
            call adios2_attribute_length_f2c(attribute%length, attribute%f2c, &
                                             ierr)
            call adios2_attribute_is_value_f2c(is_valueInt, attribute%f2c, ierr)

            if(is_valueInt == 0) then
                attribute%is_value = .false.
            else
                attribute%is_value = .true.
            end if

        else
            attribute%valid = .false.
            attribute%name = ''
            attribute%type = adios2_type_unknown
            attribute%length = 0
        end if

    end subroutine


    subroutine adios2_remove_attribute(result, io, name, ierr)
        type(adios2_io), intent(in) :: io
        character*(*), intent(in) :: name
        logical, intent(out) :: result
        integer, intent(out) :: ierr
        ! Local
        type(adios2_attribute):: attribute
        integer :: resultInt

        call adios2_remove_attribute_f2c(resultInt, io%f2c, &
                                         TRIM(ADJUSTL(name))//char(0), ierr)
        if( resultInt == 1) then
            result = .true.
        else
            result = .false.
        end if

    end subroutine


    subroutine adios2_remove_all_attributes(io, ierr)
        type(adios2_io), intent(in) :: io
        integer, intent(out) :: ierr

        call adios2_remove_all_attributes_f2c(io%f2c, ierr)

    end subroutine


    subroutine adios2_flush_all_engines(io, ierr)
        type(adios2_io), intent(in) :: io
        integer, intent(out) :: ierr

        call adios2_flush_all_engines_f2c(io%f2c, ierr)

    end subroutine

    subroutine adios2_lock_definitions(io, ierr)
        type(adios2_io), intent(in) :: io
        integer, intent(out) :: ierr

        call adios2_lock_definitions_f2c(io%f2c, ierr)

    end subroutine

end module
