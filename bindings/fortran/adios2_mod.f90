module adios2

    #include "adios2_parameters.f90"

    ! Overload adios2_init functions
    interface adios2_init
        #ifdef ADIOS2_USE_MPI_Fortran
        module procedure adios2_init_mpi
        module procedure adios2_init_config_mpi
        #else
        module procedure adios2_init_nompi
        module procedure adios2_init_config_nompi
        #endif
    end interface

        #ifdef ADIOS2_USE_MPI_Fortran
    subroutine adios2_init_mpi(adios, comm, debug_mode, ierr)
        implicit none
        integer, intent(out) :: adios
        integer, intent(in) :: comm
        logical, intent(in), optional :: debug_mode
        integer, intent(out), optional :: ierr
    end

    subroutine adios2_init_config_mpi(adios, config_file, comm, debug_mode &
        & ierr)
        implicit none
        integer, intent(out) :: adios
        character(*), intent(in) :: config_file
        integer, intent(in) :: comm
        logical, intent(in), optional :: debug_mode
        integer, intent(out), optional :: ierr
    end

        #else
    subroutine adios2_init_config_nompi(adios, config_file, debug_mode, &
        & ierr)
        implicit none
        integer, intent(out) :: adios
        character(*), intent(in) :: config_file
        logical, intent(in), optional :: debug_mode
        integer, intent(out), optional :: ierr
    end

    subroutine adios2_init_nompi(adios, debug_mode, ierr)
        implicit none
        integer, intent(out) :: adios
        logical, intent(in), optional :: debug_mode
        integer, intent(out), optional :: ierr
    end


end module

