! SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
!
! SPDX-License-Identifier: Apache-2.0


module adios2_io_open_mod

#ifdef ADIOS2_HAVE_FORTRAN_SUBMODULES
    use adios2_parameters_mod
    implicit none

    interface adios2_open

        module subroutine adios2_open_old_comm(engine, io, name, adios2_mode, ierr)
            type(adios2_engine), intent(out) :: engine
            type(adios2_io), intent(in) :: io
            character*(*), intent(in) :: name
            integer, intent(in) :: adios2_mode
            integer, intent(out) :: ierr
        end subroutine

# ifdef ADIOS2_HAVE_MPI_F

        module subroutine adios2_open_new_comm(engine, io, name, adios2_mode, comm, ierr)
            type(adios2_engine), intent(out) :: engine
            type(adios2_io), intent(in) :: io
            character*(*), intent(in) :: name
            integer, intent(in) :: adios2_mode
            integer, intent(in) :: comm
            integer, intent(out) :: ierr
        end subroutine

# endif

    end interface

#else

    use adios2_io_open_serial_mod
# ifdef ADIOS2_HAVE_MPI_F
    use adios2_io_open_mpi_mod
# endif

#endif

end module
