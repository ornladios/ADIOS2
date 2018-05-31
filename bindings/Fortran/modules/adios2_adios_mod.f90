!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_adios_mod.f90 : ADIOS2 Fortran bindings for the ADIOS component
!
!   Created on: Aug 22, 2017
!       Author: William F Godoy godoywf@ornl.gov
!

module adios2_adios_mod
    use adios2_adios_init_mod
    implicit none

contains

    subroutine adios2_declare_io(io, adios, io_name, ierr)
        type(adios2_io), intent(out) :: io
        type(adios2_adios), intent(in) :: adios
        character*(*), intent(in)  :: io_name
        integer, intent(out) :: ierr

        call adios2_declare_io_f2c(io%f2c, adios%f2c, &
                                   TRIM(ADJUSTL(io_name))//char(0), ierr)
        if(ierr == 0 ) io%valid = .true.

    end subroutine

    subroutine adios2_flush_all(adios, ierr)
        type(adios2_adios), intent(in) :: adios
        integer, intent(out) :: ierr

        call adios2_flush_all_f2c(adios%f2c, ierr)

    end subroutine

    subroutine adios2_finalize(adios, ierr)
        type(adios2_adios), intent(inout) :: adios
        integer, intent(out) :: ierr

        call adios2_finalize_f2c(adios%f2c, ierr)
        adios%valid = .false.

    end subroutine

end module
