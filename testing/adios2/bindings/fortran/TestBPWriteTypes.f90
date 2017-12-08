program TestBPWriteTypes
    use small_test_data
    use mpi
    use adios2
    implicit none

    integer, dimension(1) :: shape_dims, start_dims, count_dims
    integer :: inx, irank, isize, ierr, i

    integer(kind=8) :: adios, io, engine
    integer(kind=8), dimension(6) :: variables

    ! Launch MPI
    call MPI_Init(ierr)
    call MPI_Comm_rank(MPI_COMM_WORLD, irank, ierr)
    call MPI_Comm_size(MPI_COMM_WORLD, isize, ierr)

    ! Application variables
    inx = 10

    ! Variable dimensions
    shape_dims(1) = isize * inx
    start_dims(1) = irank * inx
    count_dims(1) = inx

    ! Create adios handler passing the communicator, debug mode and error flag
    call adios2_init(adios, MPI_COMM_WORLD, adios2_debug_mode_on, ierr)

    ! Declare an IO process configuration inside adios
    call adios2_declare_io(io, adios, "bpIO", ierr)

    ! Defines a variable to be written in bp format
    call adios2_define_variable(variables(1), io, "var_I8", adios2_type_integer1, 1, &
        & shape_dims, start_dims, count_dims, adios2_constant_dims_true, ierr)

    ! Open myVector_f.bp in write mode, this launches an engine
    call adios2_open(engine, io, "ftypes.bp", adios2_mode_write, ierr)

    ! Put array contents to bp buffer, based on var1 metadata
    call adios2_put_sync(engine, variables(1), data_I8, ierr)

    ! Closes engine1 and deallocates it, becomes unreachable
    call adios2_close(engine, ierr)

    ! Deallocates adios and calls its destructor
    call adios2_finalize(adios, ierr)

    call MPI_Finalize(ierr)

end program TestBPWriteTypes
