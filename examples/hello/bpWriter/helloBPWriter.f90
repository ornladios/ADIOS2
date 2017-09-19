program helloBPWriter
    use mpi
    use adios2

    implicit none

    integer, dimension(1) :: shape_dims, start_dims, count_dims
    real, dimension(:), allocatable :: myArray
    integer :: inx, irank, isize, ierr, i
    integer(kind=8) :: adios, io, var1, engine1

    ! Launch MPI
    call MPI_Init(ierr)
    call MPI_Comm_rank(MPI_COMM_WORLD, irank, ierr)
    call MPI_Comm_size(MPI_COMM_WORLD, isize, ierr)

    ! Application variables
    inx = 10
    allocate( myArray(inx) )

    do i=1,inx
        myArray(i) = i-1
    end do

    ! Variable dimensions
    shape_dims(1) = isize * inx
    start_dims(1) = irank * inx
    count_dims(1) = inx

    ! Create adios handler passing the communicator, debug mode and error flag
    call adios2_init(adios, MPI_COMM_WORLD, adios2_debug_mode_on, ierr)

    ! Declare an IO process configuration inside adios
    call adios2_declare_io(io, adios, "bpIO", ierr)

    ! Defines a variable to be written in bp format
    call adios2_define_variable(var1, io, "myArray", adios2_type_real, 1, &
        & shape_dims, start_dims, count_dims, adios2_constant_dims_true, ierr)

    ! Open myVector_f.bp in write mode, this launches an engine
    call adios2_open(engine1, io, "myVector_f.bp", adios2_open_mode_write, ierr)

    ! Write myArray contents to bp buffer, based on var1 metadata
    call adios2_write(engine1, var1, myArray, ierr)

    ! Closes engine1 and deallocates it, becomes unreachable
    call adios2_close(engine1, ierr)

    ! Deallocates adios and calls its destructor
    call adios2_finalize(adios, ierr)

    deallocate(myArray)

    call MPI_Finalize(ierr)

end program helloBPWriter
