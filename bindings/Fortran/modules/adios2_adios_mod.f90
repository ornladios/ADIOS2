!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_adios_mod.f90 : ADIOS2 Fortran bindings for the ADIOS component
!
!   Created on: Aug 22, 2017
!       Author: William F Godoy godoywf@ornl.gov
!

module adios2_adios
    use adios2_adios_init
    implicit none

contains

    subroutine adios2_declare_io(io, adios, io_name, ierr)
        integer(kind=8), intent(out) :: io
        integer(kind=8), intent(in) :: adios
        character*(*), intent(in)  :: io_name
        integer, intent(out) :: ierr

        call adios2_declare_io_f2c(io, adios, TRIM(ADJUSTL(io_name))//char(0), &
                                   ierr)

    end subroutine

    subroutine adios2_flush_all(adios, ierr)
        integer(kind=8), intent(in) :: adios
        integer, intent(out) :: ierr

        call adios2_flush_all_f2c(adios, ierr)

    end subroutine

    subroutine adios2_finalize(adios, ierr)
        integer(kind=8), intent(in) :: adios
        integer, intent(out) :: ierr

        call adios2_finalize_f2c(adios, ierr)

    end subroutine

end module
