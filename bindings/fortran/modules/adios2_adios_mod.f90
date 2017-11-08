
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
        & ierr)

    end


    subroutine adios2_finalize(adios, ierr)
        integer(kind=8), intent(in) :: adios
        integer, intent(out) :: ierr

        call adios2_finalize_f2c(adios, ierr)
    end


end module
